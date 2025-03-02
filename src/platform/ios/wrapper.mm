#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include "application.h"

WindowDelegatePtr createWindowDelegate(CAMetalLayer* metalLayer);

@interface MetalView : UIView
@end

@implementation MetalView
+ (Class)layerClass {
  return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    CAMetalLayer* metalLayer = (CAMetalLayer*)self.layer;
    metalLayer.device = MTLCreateSystemDefaultDevice();
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.framebufferOnly = YES;
  }
  return self;
}
@end

@interface ViewController : UIViewController
@property (strong, nonatomic) CADisplayLink* displayLink;
@end

@implementation ViewController {
  ApplicationPtr _application;
}

- (void)loadView {
  self.view = [[MetalView alloc] initWithFrame:[UIScreen mainScreen].bounds];
}

- (void)viewDidLoad {
  [super viewDidLoad];

  NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
  const char* bundlePathCStr = [bundlePath UTF8String];

  _application = createApplication(bundlePathCStr,
    createWindowDelegate((CAMetalLayer*)self.view.layer));

  self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(updateLoop:)];
  [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)updateLoop:(CADisplayLink*)sender {
  _application->update();
}

- (void)dealloc {
  [self.displayLink invalidate];
  [super dealloc];
}
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation AppDelegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions {
  self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
  self.window.rootViewController = [[ViewController alloc] init];
  [self.window makeKeyAndVisible];
  return YES;
}
@end

int main(int argc, char* argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
