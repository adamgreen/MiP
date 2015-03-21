/*
     File: HeartRateMonitorAppDelegate.m
 Abstract: Implementatin of Heart Rate Monitor app using Bluetooth Low Energy (LE) Heart Rate Service. This app demonstrats the use of CoreBluetooth APIs for LE devices.
  Version: 1.0
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2011 Apple Inc. All Rights Reserved.
 
 */

#import "HeartRateMonitorAppDelegate.h"
#import <QuartzCore/QuartzCore.h>

@implementation HeartRateMonitorAppDelegate

@synthesize window;
@synthesize heartRate;
@synthesize heartView;
@synthesize pulseTimer;
@synthesize scanSheet;
@synthesize heartRateMonitors;
@synthesize arrayController;
@synthesize manufacturer;
@synthesize connected;

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

/* MiP Protocol Commands */
#define MIP_CMD_GET_SOFTWARE_VERSION 0x14

#define PULSESCALE 1.2
#define PULSEDURATION 0.2


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

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    self.heartRate = 0;
    characteristicsToFind = 2;
    autoConnect = FALSE;
    self.heartRateMonitors = [NSMutableArray array];
       
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:0.];
    [self.heartView layer].position = CGPointMake( [[self.heartView layer] frame].size.width / 2, [[self.heartView layer] frame].size.height / 2 );
    [self.heartView layer].anchorPoint = CGPointMake(0.5, 0.5);
    [NSAnimationContext endGrouping];

    manager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
    if( autoConnect )
    {
        [self startScan];
    }
}

- (void) dealloc
{
    [self stopScan];
    
    [peripheral setDelegate:nil];
    [peripheral release];
    
    [heartRateMonitors release];
        
    [manager release];
    
    [super dealloc];
}

/* 
 Disconnect peripheral when application terminate 
*/
- (void) applicationWillTerminate:(NSNotification *)notification
{
    if(peripheral)
    {
        [manager cancelPeripheralConnection:peripheral];
    }
}

#pragma mark - Scan sheet methods

/* 
 Open scan sheet to discover heart rate peripherals if it is LE capable hardware 
*/
- (IBAction)openScanSheet:(id)sender 
{
    if( [self isLECapableHardware] )
    {
        autoConnect = FALSE;
        [arrayController removeObjects:heartRateMonitors];
        [NSApp beginSheet:self.scanSheet modalForWindow:self.window modalDelegate:self didEndSelector:@selector(sheetDidEnd:returnCode:contextInfo:) contextInfo:nil];
        [self startScan];
    }
}

/*
 Close scan sheet once device is selected
*/
- (IBAction)closeScanSheet:(id)sender 
{
    [NSApp endSheet:self.scanSheet returnCode:NSAlertDefaultReturn];
    [self.scanSheet orderOut:self];    
}

/*
 Close scan sheet without choosing any device
*/
- (IBAction)cancelScanSheet:(id)sender 
{
    [NSApp endSheet:self.scanSheet returnCode:NSAlertAlternateReturn];
    [self.scanSheet orderOut:self];
}

/* 
 This method is called when Scan sheet is closed. Initiate connection to selected MiP robot.
*/
- (void)sheetDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo 
{
    [self stopScan];
    if( returnCode == NSAlertDefaultReturn )
    {
        NSIndexSet *indexes = [self.arrayController selectionIndexes];
        if ([indexes count] != 0) 
        {
            NSUInteger anIndex = [indexes firstIndex];
            peripheral = [self.heartRateMonitors objectAtIndex:anIndex];
            [peripheral retain];
            [indicatorButton setHidden:FALSE];
            [progressIndicator setHidden:FALSE];
            [progressIndicator startAnimation:self];
            [connectButton setTitle:@"Cancel"];
            [manager connectPeripheral:peripheral options:nil];
        }
    }
}

#pragma mark - Connect Button

/*
 This method is called when connect button pressed and it takes appropriate actions depending on device connection state
 */
- (IBAction)connectButtonPressed:(id)sender
{
    if(peripheral && (peripheral.state == CBPeripheralStateConnected))
    { 
        /* Disconnect if it's already connected */
        [manager cancelPeripheralConnection:peripheral]; 
    }
    else if (peripheral)
    {
        /* Device is not connected, cancel pendig connection */
        [indicatorButton setHidden:TRUE];
        [progressIndicator setHidden:TRUE];
        [progressIndicator stopAnimation:self];
        [connectButton setTitle:@"Connect"];
        [manager cancelPeripheralConnection:peripheral];
        [self openScanSheet:nil];
    }
    else
    {   /* No outstanding connection, open scan sheet */
        [self openScanSheet:nil];
    }
}

#pragma mark - Heart Rate Data

/* 
 Update UI with heart rate data received from device
 */
- (void) updateWithHRMData:(NSData *)data 
{
    const uint8_t *reportData = [data bytes];
    uint16_t bpm = 0;
    
    if ((reportData[0] & 0x01) == 0) 
    {
        /* uint8 bpm */
        bpm = reportData[1];
    } 
    else 
    {
        /* uint16 bpm */
        bpm = CFSwapInt16LittleToHost(*(uint16_t *)(&reportData[1]));
    }
    
    uint16_t oldBpm = self.heartRate;
    self.heartRate = bpm;
    if (oldBpm == 0) 
    {
        [self pulse];
        self.pulseTimer = [NSTimer scheduledTimerWithTimeInterval:(60. / heartRate) target:self selector:@selector(pulse) userInfo:nil repeats:NO];
    }
}

/*
 Update pulse UI
 */
- (void) pulse 
{
    CABasicAnimation *pulseAnimation = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    
    pulseAnimation.toValue = [NSNumber numberWithFloat:PULSESCALE];
    pulseAnimation.fromValue = [NSNumber numberWithFloat:1.0];
    
    pulseAnimation.duration = PULSEDURATION;
    pulseAnimation.repeatCount = 1;
    pulseAnimation.autoreverses = YES;
    pulseAnimation.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
    
    [[heartView layer] addAnimation:pulseAnimation forKey:@"scale"];
    
    self.pulseTimer = [NSTimer scheduledTimerWithTimeInterval:(60. / heartRate) target:self selector:@selector(pulse) userInfo:nil repeats:NO];
}

#pragma mark - Start/Stop Scan methods

/*
 Uses CBCentralManager to check whether the current platform/hardware supports Bluetooth LE. An alert is raised if Bluetooth LE is not enabled or is not supported.
 */
- (BOOL) isLECapableHardware
{
    NSString * state = nil;
    
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
            return TRUE;
        case CBCentralManagerStateUnknown:
        default:
            return FALSE;
            
    }
    
    NSLog(@"Central manager state: %@", state);
    
    [self cancelScanSheet:nil];
    
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    [alert setMessageText:state];
    [alert addButtonWithTitle:@"OK"];
    [alert setIcon:[[[NSImage alloc] initWithContentsOfFile:@"AppIcon"] autorelease]];
    [alert beginSheetModalForWindow:[self window] modalDelegate:self didEndSelector:nil contextInfo:nil];
    return FALSE;
}

/*
 Request CBCentralManager to scan for WowWee MiP via one of the two services that it broadcasts.
 */
- (void) startScan
{
    [manager scanForPeripheralsWithServices:[NSArray arrayWithObject:[CBUUID UUIDWithString:@MIP_BROADCAST_SERVICE1]] options:nil];
}

/*
 Request CBCentralManager to stop scanning for heart rate peripherals
 */
- (void) stopScan 
{
    [manager stopScan];
}

#pragma mark - CBCentralManager delegate methods
/*
 Invoked whenever the central manager's state is updated.
 */
- (void) centralManagerDidUpdateState:(CBCentralManager *)central 
{
    [self isLECapableHardware];
}
    
/*
 Invoked when the central discovers heart rate peripheral while scanning.
 */
- (void) centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)aPeripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI 
{
    /* Check the manufacturing data to make sure that the first two bytes are 0x00 0x05 to indicate that it is a MiP device. */
    NSData* manufacturerDataObject = [advertisementData objectForKey:CBAdvertisementDataManufacturerDataKey];
    uint8_t manufacturerData[2];
    [manufacturerDataObject getBytes:manufacturerData length:sizeof(manufacturerData)];
    if (0 != memcmp(manufacturerData, MIP_MANUFACTURER_DATA_TYPE, sizeof(manufacturerData)))
    {
        return;
    }
    
    NSMutableArray *peripherals = [self mutableArrayValueForKey:@"heartRateMonitors"];
    if( ![self.heartRateMonitors containsObject:aPeripheral] )
        [peripherals addObject:aPeripheral];
    
    /* Connect to first MiP device found. */
    if(autoConnect)
    {
        NSLog(@"Auto connecting to %@", aPeripheral.name);
        peripheral = aPeripheral;
        [peripheral retain];
        [indicatorButton setHidden:FALSE];
        [progressIndicator setHidden:FALSE];
        [progressIndicator startAnimation:self];
        [connectButton setTitle:@"Cancel"];
        [manager connectPeripheral:peripheral options:nil];
    }
    
    NSLog(@"didDiscoverPeripheral: Name: %@", aPeripheral.name);
    NSLog(@"didDiscoverPeripheral: Advertisment Data: %@", advertisementData);
    NSLog(@"didDiscoverPeripheral: RSSI: %@", RSSI);
}

// UNDONE: I probably don't need this any more since I don't retrieve this list but just connect to it.
//         NOTE: I am interested in the options specified with version of connectPeripheral though.
/*
 Invoked when the central manager retrieves the list of known peripherals.
 Automatically connect to first known peripheral
 */
- (void)centralManager:(CBCentralManager *)central didRetrievePeripherals:(NSArray *)peripherals
{
    NSLog(@"Retrieved peripheral: %lu - %@", [peripherals count], peripherals);
    
    [self stopScan];
    
    /* If there are any known devices, automatically connect to it.*/
    if([peripherals count] >=1)
    {
        [indicatorButton setHidden:FALSE];
        [progressIndicator setHidden:FALSE];
        [progressIndicator startAnimation:self];
        peripheral = [peripherals objectAtIndex:0];
        [peripheral retain];
        [connectButton setTitle:@"Cancel"];
        [manager connectPeripheral:peripheral options:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:CBConnectPeripheralOptionNotifyOnDisconnectionKey]];
    }
}

/*
 Invoked whenever a connection is succesfully created with the peripheral. 
 Discover available services on the peripheral
 */
- (void) centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)aPeripheral 
{
    characteristicsToFind = 2;
    [aPeripheral setDelegate:self];
    [aPeripheral discoverServices:nil];
	
	self.connected = @"Connected";
    [connectButton setTitle:@"Disconnect"];
    [indicatorButton setHidden:TRUE];
    [progressIndicator setHidden:TRUE];
    [progressIndicator stopAnimation:self];
}

/*
 Invoked whenever an existing connection with the peripheral is torn down. 
 Reset local variables
 */
- (void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)aPeripheral error:(NSError *)error
{
	self.connected = @"Not connected";
    [connectButton setTitle:@"Connect"];
    self.manufacturer = @"";
    self.heartRate = 0;
    if( peripheral )
    {
        [peripheral setDelegate:nil];
        [peripheral release];
        peripheral = nil;
    }
}

/*
 Invoked whenever the central manager fails to create a connection with the peripheral.
 */
- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)aPeripheral error:(NSError *)error
{
    NSLog(@"Fail to connect to peripheral: %@ with error = %@", aPeripheral, [error localizedDescription]);
    [connectButton setTitle:@"Connect"]; 
    if( peripheral )
    {
        [peripheral setDelegate:nil];
        [peripheral release];
        peripheral = nil;
    }
}

#pragma mark - CBPeripheral delegate methods
/*
 Invoked upon completion of a -[discoverServices:] request.
 Discover available characteristics on interested services
 */
- (void) peripheral:(CBPeripheral *)aPeripheral didDiscoverServices:(NSError *)error 
{
    for (CBService *aService in aPeripheral.services) 
    {
        NSLog(@"Service found with UUID: %@", aService.UUID);
        
        /* MiP specific services */
        if ([aService.UUID isEqual:[CBUUID UUIDWithString:@MIP_RECEIVE_DATA_SERVICE]] ||
            [aService.UUID isEqual:[CBUUID UUIDWithString:@MIP_SEND_DATA_SERVICE]])
        {
            [aPeripheral discoverCharacteristics:nil forService:aService];
        }
        
        /* Device Information Service */
        if ([aService.UUID isEqual:[CBUUID UUIDWithString:@"180A"]]) 
        {
            [aPeripheral discoverCharacteristics:nil forService:aService];
        }
        
        /* GAP (Generic Access Profile) for Device Name */
        if ( [aService.UUID isEqual:[CBUUID UUIDWithString:CBUUIDGenericAccessProfileString]] )
        {
            [aPeripheral discoverCharacteristics:nil forService:aService];
        }
    }
}

/*
 Invoked upon completion of a -[discoverCharacteristics:forService:] request.
 Perform appropriate operations on interested characteristics
 */
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
                characteristicsToFind--;
                NSLog(@"Found a Receive Data Notify Characteristic");
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
                NSLog(@"Found a Send Data Write Characteristic");
                characteristicsToFind--;
            }
        }
    }
    
    if ( [service.UUID isEqual:[CBUUID UUIDWithString:CBUUIDGenericAccessProfileString]] )
    {
        for (CBCharacteristic *aChar in service.characteristics) 
        {
            /* Read device name */
            if ([aChar.UUID isEqual:[CBUUID UUIDWithString:CBUUIDDeviceNameString]]) 
            {
                [aPeripheral readValueForCharacteristic:aChar];
                NSLog(@"Found a Device Name Characteristic");
            }
        }
    }
    
    if ([service.UUID isEqual:[CBUUID UUIDWithString:@"180A"]]) 
    {
        for (CBCharacteristic *aChar in service.characteristics) 
        {
            /* Read manufacturer name */
            if ([aChar.UUID isEqual:[CBUUID UUIDWithString:@"2A29"]]) 
            {
                [aPeripheral readValueForCharacteristic:aChar];
                NSLog(@"Found a Device Manufacturer Name Characteristic");
            }
        }
    }

    if (characteristicsToFind == 0)
    {
        uint8_t cmd = MIP_CMD_GET_SOFTWARE_VERSION;
        NSData* cmdData = [NSData dataWithBytes:(void*)&cmd length:sizeof(cmd)];
        [aPeripheral writeValue:cmdData forCharacteristic:sendDataWriteCharacteristic type:CBCharacteristicWriteWithResponse];
        characteristicsToFind = -1;
    }
}

/*
 Invoked upon completion of a -[readValueForCharacteristic:] request or on the reception of a notification/indication.
 */
- (void) peripheral:(CBPeripheral *)aPeripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error 
{
    /* Value for device Name received */
    if ([characteristic.UUID isEqual:[CBUUID UUIDWithString:CBUUIDDeviceNameString]])
    {
        NSString * deviceName = [[[NSString alloc] initWithData:characteristic.value encoding:NSUTF8StringEncoding] autorelease];
        NSLog(@"Device Name = %@", deviceName);
    } 
    /* Value for manufacturer name received */
    else if ([characteristic.UUID isEqual:[CBUUID UUIDWithString:@"2A29"]]) 
    {
        self.manufacturer = [[[NSString alloc] initWithData:characteristic.value encoding:NSUTF8StringEncoding] autorelease];
        NSLog(@"Manufacturer Name = %@", self.manufacturer);
    }
    /* Response from MiP command has been received. */
    else if ([characteristic.UUID isEqual:[CBUUID UUIDWithString:@MIP_RECEIVE_DATA_NOTIFY_CHARACTERISTIC]])
    {
        const uint8_t* pResponseBytes = characteristic.value.bytes;
        NSUInteger responseLength = [characteristic.value length];
        if ((responseLength & 1) != 0)
        {
            /* Expect the response to be hexadecimal text with two characters per digit. */
            return;
        }
        /* Allocate a buffer to hold the actual bytes of the response which are parsed from hex data. */
        responseLength /= 2;
        uint8_t* pResponse = malloc(responseLength);
        if (!pResponse)
        {
            return;
        }
        /* Convert hexadecimal string into raw byte response. */
        for (int i = 0, j = 0 ; i < responseLength ; i++, j+=2)
        {
            pResponse[i] = parseHexDigit(pResponseBytes[j]) << 4 | parseHexDigit(pResponseBytes[j+1]);
        }

        if (pResponse[0] == MIP_CMD_GET_SOFTWARE_VERSION && responseLength >= 5)
        {
            NSLog(@"Get MiP Version Command Response (0x%02X)", pResponse[0]);
            NSLog(@"Year = %u", pResponse[1]);
            NSLog(@"Month = %u", pResponse[2]);
            NSLog(@"Day = %u", pResponse[3]);
            NSLog(@"Revision = %u", pResponse[4]);
        }
        else
        {
            NSLog(@"Response buffer = %@", [[[NSString alloc] initWithData:characteristic.value encoding:NSUTF8StringEncoding] autorelease]);
        }
        
        free(pResponse);
    }
}

@end
