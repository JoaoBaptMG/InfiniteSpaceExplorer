#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>

@class RootViewController;

@interface AppController : NSObject <UIApplicationDelegate, GKGameCenterControllerDelegate> {

}

@property(nonatomic, readonly) RootViewController* viewController;

@end

