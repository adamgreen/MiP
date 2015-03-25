/* Copyright (C) 2015  Adam Green (https://github.com/adamgreen)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* Implementation of MiP BLE transport for OS X using Core Bluetooth.
   It runs a NSApplication on the main thread and runs the developer's code on a worker thread [robotMain()].  This
   code is to be used with console applications on OS X.
*/
#import <Cocoa/Cocoa.h>
#import <IOBluetooth/IOBluetooth.h>
#import <pthread.h>
#import "mip.h"


// Forward Declarations.
static int parseHexDigit(uint8_t digit);
static void* robotThread(void* pArg);



// These are the services listed by MiP in it's broadcast message.
// They aren't the ones we will actually use after connecting to the device though.
#define MIP_BROADCAST_SERVICE1 "fff0"
#define MIP_BROADCAST_SERVICE2 "ffb0"

// These are the services used to send/receive data with the MiP.
#define MIP_RECEIVE_DATA_SERVICE    "ffe0"
#define MIP_SEND_DATA_SERVICE       "ffe5"

// Characteristic of MIP_RECEIVE_DATA_SERVICE which receives data from MiP.
// The controller can register for notifications on this characteristic.
#define MIP_RECEIVE_DATA_NOTIFY_CHARACTERISTIC "ffe4"
// Characteristic of MIP_SEND_DATA_SERVICE to which data is sent to MiP.
#define MIP_SEND_DATA_WRITE_CHARACTERISTIC "ffe9"

// MiP devices will have the following values in the first 2 bytes of their Manufacturer Data.
#define MIP_MANUFACTURER_DATA_TYPE "\x00\x05"



// This class contains the information for a single request and its matching response (if it has one).
@interface MiPRequestResponse : NSObject
{
    pthread_mutex_t mutex;
    pthread_cond_t  condition;
    uint8_t         waitingForResponse;
    uint8_t         requestLength;
    uint8_t         responseLength;
    uint8_t         request[MIP_REQUEST_MAX_LEN];
    uint8_t         response[MIP_RESPONSE_MAX_LEN];
}

- (id) initWithRequest:(const uint8_t*)p length:(size_t)len expectResponse:(BOOL) expectResponse;

- (void) setRequest:(const uint8_t*)p length:(size_t)len;
- (const uint8_t*) request;
- (size_t) requestLength;

- (BOOL) waitingForResponse;
- (void) waitForResponse;

- (void) setResponse:(const uint8_t*)p length:(size_t)len;
- (const uint8_t*) response;
- (size_t) responseLength;
@end



@implementation MiPRequestResponse
// Initialize the object with the desired request byte buffer and flag as expecting response or not.
// Also intitializes pthread synchronization objects so that worker thread can block and wait for a response.
- (id) initWithRequest:(const uint8_t*)p length:(size_t)len expectResponse:(BOOL) expect
{
    int ret = -1;

    self = [super init];
    if (!self)
        return nil;

    ret = pthread_mutex_init(&mutex, NULL);
    if (ret)
        return nil;
    ret = pthread_cond_init(&condition, NULL);
    if (ret)
    {
        pthread_mutex_destroy(&mutex);
        return nil;
    }

    [self setRequest:p length:len];
    waitingForResponse = expect;
    return self;
}

// Free pthread synchronization objects when this request/response object is finally freed.
- (void) dealloc
{
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);
    [super dealloc];
}

// Make a deep copy of the request.
- (void) setRequest:(const uint8_t*)p length:(size_t)len
{
    assert( len <= sizeof(request) );
    memcpy(request, p, len);
    requestLength = len;
}

// Accessor for the request byte array.
- (const uint8_t*) request
{
    return request;
}

// Accessor for the request buffer length.
- (size_t) requestLength
{
    return (size_t)requestLength;
}

// Is still waiting for a response to the last request?
- (BOOL) waitingForResponse
{
    return waitingForResponse;
}

// Block and wait for the response to the last request to actually arrive from the robot.
- (void) waitForResponse
{
    pthread_mutex_lock(&mutex);
    while (waitingForResponse)
        pthread_cond_wait(&condition, &mutex);
    pthread_mutex_unlock(&mutex);
}

// Make a deep copy of the response received from the robot.
// Also unblocks any calls to the waitForResponse selector.
- (void) setResponse:(const uint8_t*)p length:(size_t)len
{
    assert( len <= sizeof(response) );
    pthread_mutex_lock(&mutex);
        memcpy(response, p, len);
        responseLength = len;
        waitingForResponse = FALSE;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&condition);
}

// Accessor for the response byte array.
- (const uint8_t*) response
{
    return response;
}

// Accessor for the response buffer length.
- (size_t) responseLength
{
    return (size_t)responseLength;
}
@end



// This is the delegate where most of the work on the main thread occurs.
@interface MiPAppDelegate : NSObject <NSApplicationDelegate, CBCentralManagerDelegate, CBPeripheralDelegate>
{
    CBCentralManager*   manager;
    NSMutableArray*     discoveredRobots;
    CBPeripheral*       peripheral;
    CBCharacteristic*   sendDataWriteCharacteristic;

    MiPRequestResponse* requestResponse;

    int                 error;
    int32_t             characteristicsToFind;
    BOOL                autoConnect;

    pthread_mutex_t     connectMutex;
    pthread_mutex_t     oobMutex;
    pthread_cond_t      connectCondition;
    pthread_t           thread;

    // Out of band MiP responses.
    uint8_t             oobResponse[MIP_RESPONSE_MAX_LEN];
    uint8_t             oobResponseLength;
}

- (id) initForApp:(NSApplication*) app;
- (int) error;
- (void) clearPeripheral;
- (void) handleMiPConnect:(id) robotName;
- (void) foundCharacteristic;
- (void) signalConnectionError;
- (void) waitForConnectToComplete;
- (void) handleMiPDiscoveryStart:(id) dummy;
- (void) handleMiPDiscoveryStop:(id) dummy;
- (NSUInteger) getDiscoveredRobotCount;
- (NSString*) getDiscoveredRobotAtIndex:(NSUInteger) index;
- (void) handleMiPRequest:(id) request;
- (void) copyOobResponse:(uint8_t*) pOobResponse length:(size_t) len actualLength:(size_t*) pActual;
- (void) handleQuitRequest:(id) dummy;
- (void) startScan;
- (void) stopScan;
@end



@implementation MiPAppDelegate
// Initialize this delegate.
// Create necessary synchronization objects for managing worker thread's access to connection and response state.
// Also adds itself as the delegate to the main NSApplication object.
- (id) initForApp:(NSApplication*) app;
{
    int ret = -1;
    int responseInit = -1;
    int notificationInit = -1;

    self = [super init];
    if (!self)
        return nil;

    discoveredRobots = [[NSMutableArray alloc] init];
    if (!discoveredRobots)
        return nil;

    ret = pthread_mutex_init(&connectMutex, NULL);
    if (ret)
        return nil;
    ret = pthread_mutex_init(&oobMutex, NULL);
    if (ret)
    {
        pthread_mutex_destroy(&connectMutex);
        return nil;
    }
    ret = pthread_cond_init(&connectCondition, NULL);
    if (ret)
    {
        pthread_mutex_destroy(&oobMutex);
        pthread_mutex_destroy(&connectMutex);
        return nil;
    }

    [app setDelegate:self];
    return self;
}


// Invoked when application finishes launching.
// Initialize the Core Bluetooth manager object and also starts up the worker thread.  This worker thread will end up
// running the code in the developer's robotMain() implementation.
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    manager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
    pthread_create(&thread, NULL, robotThread, self);
}

// Invoked just before application will shutdown.
- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    // Stop any BLE discovery process that might have been taking place.
    [self stopScan];

    // Disconnect from the robot if necessary.
    if(peripheral)
    {
        [manager cancelPeripheralConnection:peripheral];
        [self clearPeripheral];
    }

    // Free up resources here rather than dealloc which doesn't appear to be called during NSApplication shutdown.
    [discoveredRobots release];
    discoveredRobots = nil;

    [manager release];
    manager = nil;

    pthread_cond_destroy(&connectCondition);
    pthread_mutex_destroy(&connectMutex);
    pthread_mutex_destroy(&oobMutex);
}

// Request CBCentralManager to stop scanning for MiP robots.
- (void) stopScan
{
    [manager stopScan];
}

// Clear BLE peripheral member.
- (void) clearPeripheral
{
    if (!peripheral)
        return;

    [peripheral setDelegate:nil];
    [peripheral release];
    peripheral = nil;
}

// Handle MiP robot connection request posted to the main thread by the worker thread.
- (void) handleMiPConnect:(id) robotName
{
    error = MIP_ERROR_NONE;
    characteristicsToFind = -1;
    if (discoveredRobots.count > 0)
    {
        // A discovery scan has already been completed so use the list of discovered bots.
        CBPeripheral* robot = nil;
        if (robotName == nil)
        {
            // Just use the first item in the discovered robot list.
            robot = [discoveredRobots objectAtIndex:0];
        }
        else
        {
            // Find the specified robot in the list of discovered robots.
            for (NSUInteger robotIndex = 0 ; robotIndex < discoveredRobots.count ; robotIndex++)
            {
                robot = [discoveredRobots objectAtIndex:robotIndex];
                if ([(NSString*)robotName compare:robot.name] == NSOrderedSame)
                    break;
            }
        }
        // Make sure that the specified robotName is valid.
        if (!robot)
        {
            error = MIP_ERROR_PARAM;
            return;
        }

        // Connect to specified robot.
        NSLog(@"Connecting to %@", robot.name);
        [self stopScan];
        autoConnect = FALSE;
        characteristicsToFind = 2;
        peripheral = robot;
        [peripheral retain];
        [manager connectPeripheral:peripheral options:nil];
    }
    else if (robotName == nil)
    {
        autoConnect = TRUE;
        characteristicsToFind = 2;
        [self startScan];
    }
    else
    {
        // Can't specify a robotName without first discovering near robots.
        error = MIP_ERROR_PARAM;
        return;
    }
}

// Request CBCentralManager to scan for WowWee MiP robots via one of the two services that it broadcasts.
- (void) startScan
{
    [manager scanForPeripheralsWithServices:[NSArray arrayWithObject:[CBUUID UUIDWithString:@MIP_BROADCAST_SERVICE1]] options:nil];
}

// Invoked when the central discovers MiP robots while scanning.
- (void) centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)aPeripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI
{
    // Check the manufacturing data to make sure that the first two bytes are 0x00 0x05 to indicate that it is a MiP device.
    NSData* manufacturerDataObject = [advertisementData objectForKey:CBAdvertisementDataManufacturerDataKey];
    uint8_t manufacturerData[2];
    [manufacturerDataObject getBytes:manufacturerData length:sizeof(manufacturerData)];
    if (0 != memcmp(manufacturerData, MIP_MANUFACTURER_DATA_TYPE, sizeof(manufacturerData)))
    {
        return;
    }

    // Add to discoveredRobots array if not already present in that list.
    @synchronized(discoveredRobots)
    {
        if (![discoveredRobots containsObject:aPeripheral])
            [discoveredRobots addObject:aPeripheral];
    }

    // If the user wants to connect to first discovered robot then issue connection request now.
    if (autoConnect)
    {
        // Connect to first MiP device found.
        NSLog(@"Auto connecting to %@", aPeripheral.name);
        [self stopScan];
        autoConnect = FALSE;
        peripheral = aPeripheral;
        [peripheral retain];
        [manager connectPeripheral:peripheral options:nil];
    }
}

// Invoked whenever a connection is succesfully created with a MiP robot.
// Start discovering available BLE services on the robot.
- (void) centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)aPeripheral
{
    [aPeripheral setDelegate:self];
    [aPeripheral discoverServices:[NSArray arrayWithObjects:[CBUUID UUIDWithString:@MIP_RECEIVE_DATA_SERVICE],
                                                            [CBUUID UUIDWithString:@MIP_SEND_DATA_SERVICE], nil]];
}

// Invoked whenever an existing connection with the peripheral is torn down.
- (void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)aPeripheral error:(NSError *)error
{
    [self clearPeripheral];
}

// Invoked whenever the central manager fails to create a connection with the peripheral.
- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)aPeripheral error:(NSError *)error
{
    [self clearPeripheral];
    [self signalConnectionError];
}

// Error was encountered while attempting to connect to robot.
// Record this error and unblock worker thread which is waiting for the connection to complete.
- (void) signalConnectionError
{
    pthread_mutex_lock(&connectMutex);
        characteristicsToFind = -1;
        error = MIP_ERROR_CONNECT;
    pthread_mutex_unlock(&connectMutex);
    pthread_cond_signal(&connectCondition);
}

// Invoked upon completion of a -[discoverServices:] request.
// Discover available characteristics on interested services.
- (void) peripheral:(CBPeripheral *)aPeripheral didDiscoverServices:(NSError *)error
{
    for (CBService *aService in aPeripheral.services)
    {
        /* MiP specific services */
        if ([aService.UUID isEqual:[CBUUID UUIDWithString:@MIP_RECEIVE_DATA_SERVICE]] ||
            [aService.UUID isEqual:[CBUUID UUIDWithString:@MIP_SEND_DATA_SERVICE]])
        {
            [aPeripheral discoverCharacteristics:nil forService:aService];
        }
    }
}

// Invoked upon completion of a -[discoverCharacteristics:forService:] request.
// Perform appropriate operations on interested characteristics.
- (void) peripheral:(CBPeripheral *)aPeripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error
{
    /* MiP Receive Data Service. */
    if ([service.UUID isEqual:[CBUUID UUIDWithString:@MIP_RECEIVE_DATA_SERVICE]])
    {
        for (CBCharacteristic *aChar in service.characteristics)
        {
            /* Set notification on received data. */
            if ([aChar.UUID isEqual:[CBUUID UUIDWithString:@MIP_RECEIVE_DATA_NOTIFY_CHARACTERISTIC]])
            {
                [peripheral setNotifyValue:YES forCharacteristic:aChar];
                [self foundCharacteristic];
            }
        }
    }

    /* MiP Send Data Service. */
    if ([service.UUID isEqual:[CBUUID UUIDWithString:@MIP_SEND_DATA_SERVICE]])
    {
        for (CBCharacteristic *aChar in service.characteristics)
        {
            /* Remember Send Data Characteristic pointer. */
            if ([aChar.UUID isEqual:[CBUUID UUIDWithString:@MIP_SEND_DATA_WRITE_CHARACTERISTIC]])
            {
                sendDataWriteCharacteristic = aChar;
                [self foundCharacteristic];
            }
        }
    }
}

// Found one of the two characteristics required for communicating with the MiP robot.
// The worker thread will be waiting for both of these characteristics to be found so there is code to unblock it.
- (void) foundCharacteristic
{
    pthread_mutex_lock(&connectMutex);
        characteristicsToFind--;
    pthread_mutex_unlock(&connectMutex);
    pthread_cond_signal(&connectCondition);
}

// The worker thread calls this selector to wait for the connection to the robot to complete.
- (void) waitForConnectToComplete
{
    pthread_mutex_lock(&connectMutex);
        while (characteristicsToFind > 0)
            pthread_cond_wait(&connectCondition, &connectMutex);
    pthread_mutex_unlock(&connectMutex);
}

// The worker thread calls this selector to determine if the main thread has encountered an error.
- (int) error
{
    return error;
}

// Handle MiP robot discovery start request posted to the main thread by the worker thread.
- (void) handleMiPDiscoveryStart:(id) dummy
{
    error = MIP_ERROR_NONE;
    autoConnect = FALSE;
    [self startScan];
}

// Handle MiP robot discovery stop request posted to the main thread by the worker thread.
- (void) handleMiPDiscoveryStop:(id) dummy
{
    [self stopScan];
}

// The worker thread calls this selector to determine how many MiP robots have been discovered so far.
- (NSUInteger) getDiscoveredRobotCount
{
    NSUInteger count = 0;
    @synchronized(discoveredRobots)
    {
        count = [discoveredRobots count];
    }
    return count;
}

// The worker thread calls this selector to obtain the name for one of the MiP robots discovered so far.
- (NSString*) getDiscoveredRobotAtIndex:(NSUInteger) index
{
    CBPeripheral* p = nil;
    @synchronized(discoveredRobots)
    {
        p = [discoveredRobots objectAtIndex:index];
    }
    return p.name;
}

// Handle MiP command request posted to the main thread by the worker thread.
- (void) handleMiPRequest:(id) object
{
    if (!peripheral || !sendDataWriteCharacteristic)
    {
        // Don't have a successful completion so error out.
        error = MIP_ERROR_NOT_CONNECTED;
        [object release];
        return;
    }
    error = MIP_ERROR_NONE;

    // Prepare data to send to MiP robot via Core Bluetooth.
    MiPRequestResponse* request = (MiPRequestResponse*)object;
    NSData* cmdData = [NSData dataWithBytes:[request request] length:[request requestLength]];
    if ([request waitingForResponse])
    {
        // Keep request/response object around if it expects a response to this request.
        [request retain];
        requestResponse = request;
    }

    // Send request to MiP robot via Core Bluetooth.
    [peripheral writeValue:cmdData forCharacteristic:sendDataWriteCharacteristic type:CBCharacteristicWriteWithResponse];

    // If there is no response then this release will free the object now that we don't need it anymore.
    // If there will be a response then there are already another 2 additional references to keep it alive until the
    // response has been received and copied into the worker threads' buffer.
    [object release];
}

// Invoked upon completion of a -[readValueForCharacteristic:] request or on the reception of a notification/indication.
- (void) peripheral:(CBPeripheral *)aPeripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    // Response from MiP command has been received.
    if ([characteristic.UUID isEqual:[CBUUID UUIDWithString:@MIP_RECEIVE_DATA_NOTIFY_CHARACTERISTIC]])
    {
        const uint8_t* pResponseBytes = characteristic.value.bytes;
        NSUInteger responseLength = [characteristic.value length];
        if ((responseLength & 1) != 0)
        {
            // Expect the response to be hexadecimal text with two characters per digit.
            return;
        }

        // Buffer to hold the actual bytes of the response which are parsed from hex data.
        uint8_t response[MIP_RESPONSE_MAX_LEN];
        responseLength /= 2;
        if (responseLength > sizeof(response))
            responseLength = sizeof(response);

        // Convert hexadecimal string into raw byte response.
        for (int i = 0, j = 0 ; i < responseLength ; i++, j+=2)
        {
            response[i] = parseHexDigit(pResponseBytes[j]) << 4 | parseHexDigit(pResponseBytes[j+1]);
        }

        if (requestResponse && [requestResponse request][0] == response[0])
        {
            // Have received the response for the currently pending request.
            [requestResponse setResponse:response length:responseLength];
            [requestResponse release];
            requestResponse = nil;
        }
        else
        {
            // Received Out of Band response from MiP.
            pthread_mutex_lock(&oobMutex);
                memcpy(oobResponse, response, responseLength);
                oobResponseLength = responseLength;
            pthread_mutex_unlock(&oobMutex);
        }
    }
}

// Convert hexadecimal digit text to value.
static int parseHexDigit(uint8_t digit)
{
    uint8_t ch = tolower(digit);
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    else
    {
        return 0;
    }
}

// Handle application shutdown request posted to the main thread by the worker thread.
- (void) handleQuitRequest:(id) dummy
{
    [NSApp terminate:self];
}

// The worker thread calls this selector to make a deep copy of the out of band response data.  These out of band
// responses are notifications sent from MiP robot even though no explicit request has been made.
- (void) copyOobResponse:(uint8_t*) pOobResponse length:(size_t) len actualLength:(size_t*) pActual
{
    pthread_mutex_lock(&oobMutex);
        size_t length = oobResponseLength;
        if (length > len)
            length = len;
        memcpy(pOobResponse, oobResponse, length);
        *pActual = length;
    pthread_mutex_unlock(&oobMutex);
}

// Invoked whenever the central manager's state is updated.
- (void) centralManagerDidUpdateState:(CBCentralManager *)central
{
    NSString * state = nil;

    // Display an error to user if there is no BLE hardware and then force an exit.
    switch ([manager state])
    {
        case CBCentralManagerStateUnsupported:
            state = @"The platform/hardware doesn't support Bluetooth Low Energy.";
            break;
        case CBCentralManagerStateUnauthorized:
            state = @"The app is not authorized to use Bluetooth Low Energy.";
            break;
        case CBCentralManagerStatePoweredOff:
            state = @"Bluetooth is currently powered off.";
            break;
        case CBCentralManagerStatePoweredOn:
        case CBCentralManagerStateUnknown:
        default:
            return;
    }

    NSLog(@"Central manager state: %@", state);
    [NSApp terminate:self];
}
@end



// *** Implementation of lower level transport C APIs that make use of above Objective-C classes. ***
static MiPAppDelegate* g_appDelegate;

// Initialize the MiP transport on OS X to use BLE (Bluetooth Low Energy).
// * It should be called from a console application's main().
// * It initializes the low level transport layer and starts a separate thread to run the developer's robot code.  The
//   developer provides this code in their implementation of the robotMain() function.
void osxMiPInitAndRun(void)
{
    [NSApplication sharedApplication];
    g_appDelegate = [[MiPAppDelegate alloc] initForApp:NSApp];
    [NSApp run];
    [g_appDelegate release];
    return;
}

// Worker thread root function.
// Calls developer's robotMain() function and upon return sends the Quit request to the main application thread.
static void* robotThread(void* pArg)
{
    robotMain();
    [g_appDelegate performSelectorOnMainThread:@selector(handleQuitRequest:) withObject:nil waitUntilDone:YES];
    return NULL;
}


MiPTransport* mipTransportInit(const char* pInitOptions)
{
    // Don't actually need a MiPTransport object so just return non-null value and cast appropriately.
    return (MiPTransport*)1;
}

void mipTransportUninit(MiPTransport* pTransport)
{
    // Nothing to do here.
    return;
}

int mipTransportConnectToRobot(MiPTransport* pTransport, const char* pRobotName)
{
    NSString* robotNameObject = nil;

    if (pRobotName)
        robotNameObject = [NSString stringWithUTF8String:pRobotName];
    [g_appDelegate performSelectorOnMainThread:@selector(handleMiPConnect:) withObject:robotNameObject waitUntilDone:YES];
    [g_appDelegate waitForConnectToComplete];
    [robotNameObject release];

    return [g_appDelegate error];
}

int mipTransportStartRobotDiscovery(MiPTransport* pTransport)
{
    [g_appDelegate performSelectorOnMainThread:@selector(handleMiPDiscoveryStart:) withObject:nil waitUntilDone:YES];
    return [g_appDelegate error];
}

int mipTransportGetDiscoveredRobotCount(MiPTransport* pTransport, size_t* pCount)
{
    NSUInteger count = [g_appDelegate getDiscoveredRobotCount];
    *pCount = (size_t)count;
    return [g_appDelegate error];
}

int mipTransportGetDiscoveredRobotName(MiPTransport* pTransport, size_t robotIndex, const char** ppRobotName)
{
    NSString* pName = [g_appDelegate getDiscoveredRobotAtIndex:robotIndex];
    *ppRobotName = pName.UTF8String;
    return [g_appDelegate error];
}

int mipTransportStopRobotDiscovery(MiPTransport* pTransport)
{
    [g_appDelegate performSelectorOnMainThread:@selector(handleMiPDiscoveryStop:) withObject:nil waitUntilDone:YES];
    return [g_appDelegate error];
}

// Remember last request here that requires a response.
static MiPRequestResponse* g_lastRequest;

int mipTransportSendRequest(MiPTransport* pTransport, const uint8_t* pRequest, size_t requestLength, int expectResponse)
{
    MiPRequestResponse* p = [[MiPRequestResponse alloc] initWithRequest:pRequest
                                                        length:requestLength
                                                        expectResponse:expectResponse];
    if (!p)
        return MIP_ERROR_MEMORY;

    if (expectResponse)
    {
        [p retain];
        g_lastRequest = p;
    }
    [g_appDelegate performSelectorOnMainThread:@selector(handleMiPRequest:) withObject:p waitUntilDone:YES];
    return [g_appDelegate error];
}

int mipTransportGetResponse(MiPTransport* pTransport, uint8_t* pResponseBuffer, size_t responseBufferSize, size_t* pResponseLength)
{
    if (!g_lastRequest)
        return MIP_ERROR_NO_REQUEST;
    if ([g_appDelegate error])
        return [g_appDelegate error];

    [g_lastRequest waitForResponse];

    size_t srcLength = [g_lastRequest responseLength];
    size_t copyLength = srcLength;
    if (responseBufferSize < srcLength)
        copyLength = responseBufferSize;
    memcpy(pResponseBuffer, [g_lastRequest response], copyLength);
    *pResponseLength = copyLength;

    [g_lastRequest release];
    g_lastRequest = nil;

    return MIP_ERROR_NONE;
}

int mipTransportIsResponseAvailable(MiPTransport* pTransport)
{
    if (!g_lastRequest)
        return FALSE;
    return ![g_lastRequest waitingForResponse];
}

int mipTransportGetLastOutOfBandResponse(MiPTransport* pTransport, uint8_t* pResponseBuffer, size_t responseBufferSize, size_t* pResponseLength)
{
    [g_appDelegate copyOobResponse:pResponseBuffer length:responseBufferSize actualLength:pResponseLength];
    return MIP_ERROR_NONE;
}
