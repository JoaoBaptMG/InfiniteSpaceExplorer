#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>
#import <GoogleSignIn/GoogleSignIn.h>

@class RootViewController;

@interface AppController : NSObject <UIApplicationDelegate, GKGameCenterControllerDelegate, GIDSignInUIDelegate> {

}

@property(nonatomic, readonly) RootViewController* viewController;

@end

