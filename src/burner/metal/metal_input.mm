// metal_input.mm
#import <Foundation/Foundation.h>
#import <GameController/GameController.h>

extern "C" {
   // Suppose we have something like:
   //   extern struct GameInp* GameInp;
   //   extern int nGameInpCount;
   // Or your actual FBNeo input structures
}

static NSMutableArray<GCController*>* s_connectedControllers = nil;

void MetalInitGameControllerSupport() {
    s_connectedControllers = [NSMutableArray array];

    // Listen for connect/disconnect
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserverForName:GCControllerDidConnectNotification
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification* note) {
        GCController* newController = note.object;
        if (newController) {
            [s_connectedControllers addObject:newController];
            NSLog(@"Controller connected: %@", newController.vendorName);
            // Optionally configure
            [newController setPlayerIndex:0];
        }
    }];
    [center addObserverForName:GCControllerDidDisconnectNotification
                        object:nil
                         queue:nil
                    usingBlock:^(NSNotification* note) {
        GCController* disc = note.object;
        [s_connectedControllers removeObject:disc];
        NSLog(@"Controller disconnected: %@", disc.vendorName);
    }];

    // Also discover any controllers that are already connected:
    for (GCController* c in [GCController controllers]) {
        [s_connectedControllers addObject:c];
    }
}

// Example polling function
void MetalPollGameControllers() {
    // For each connected controller, read inputs
    for (GCController* c in s_connectedControllers) {
        GCExtendedGamepad* pad = c.extendedGamepad;
        if (!pad) continue;

        // read pad.buttonA.value, pad.leftThumbstick.x, etc.
        // Then convert them to FBNeo input
        float lx = pad.leftThumbstick.xAxis.value;
        float ly = pad.leftThumbstick.yAxis.value;
        bool pressedA = (pad.buttonA.pressed);

        // TODO: map them to FBNeo's inputs
        // e.g. GameInp[0].Input.nVal = (pressedA) ? 1 : 0;
    }
}

// For keyboards, i.e. GCKeyboard:
void MetalPollKeyboard() {
    // On macOS 11+, you can do GCKeyboard *k = [GCKeyboard coalescedKeyboard];
    // Then read k.keyboardInput.buttons, etc.

    GCKeyboard* kb = [GCKeyboard coalescedKeyboard];
    if (!kb) return;
    GCKeyboardInput* kbInput = kb.keyboardInput;
    // for example: check if 'Space' is pressed
    GCControllerButtonInput* space = [kbInput buttonForKeyCode:GCKeyCodeSpacebar];
    bool isSpacePressed = space && space.pressed;

    // Then feed into FBNeo
    // e.g. GameInp[<some index>].Input.nVal = isSpacePressed ? 1 : 0;
}