#import <UIKit/UIKit.h>
#include "hello.hpp"

@interface HelloWorldAppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation HelloWorldAppDelegate

- (BOOL)application:(UIApplication*)application
  didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
  self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
  UIViewController *viewController = [[UIViewController alloc] init];

  UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 500, 500)];
  label.text = [NSString stringWithUTF8String:get_hello_message().c_str()];
  label.textAlignment = NSTextAlignmentCenter;
  label.textColor = [UIColor colorWithRed:1.0 green:1.0 blue:1.0 alpha:1.0];
  label.lineBreakMode = NSLineBreakByWordWrapping;
  label.numberOfLines = 0;
  [label sizeToFit];

  [viewController.view addSubview:label];
  self.window.rootViewController = viewController;
  [self.window makeKeyAndVisible];

  return YES;
}

@end

int main(int argc, char* argv[])
{
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([HelloWorldAppDelegate class]));
  }
}
