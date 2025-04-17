#import <UIKit/UIKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#include "application.h"

WindowDelegatePtr createWindowDelegate(CAMetalLayer* metalLayer);

@interface MetalView : UIView
@end

@implementation MetalView
+ (Class)layerClass
{
  return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame
{
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

@implementation ViewController
{
  ApplicationPtr _application;
}

- (void)loadView
{
  self.view = [[MetalView alloc] initWithFrame:[UIScreen mainScreen].bounds];
}

- (BOOL)prefersStatusBarHidden
{
  return YES;
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  CAMetalLayer* metalLayer = (CAMetalLayer*)self.view.layer;

  NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
  const char* bundlePathCStr = [bundlePath UTF8String];

  _application = createApplication(bundlePathCStr, createWindowDelegate(metalLayer));
  [self NOVA_onViewResize];

  self.view.insetsLayoutMarginsFromSafeArea = NO;
  self.view.directionalLayoutMargins = NSDirectionalEdgeInsetsZero;
  self.view.layoutMargins = UIEdgeInsetsZero;

  self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(updateLoop:)];
  [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)NOVA_onViewResize
{
  CAMetalLayer* metalLayer = (CAMetalLayer*)self.view.layer;

  CGFloat scale = [UIScreen mainScreen].scale;
  CGSize sizeInPoints = self.view.bounds.size;
  CGSize drawableSize = CGSizeMake(sizeInPoints.width * scale, sizeInPoints.height * scale);

  metalLayer.contentsScale = scale;
  metalLayer.bounds = CGRectMake(0, 0, sizeInPoints.width, sizeInPoints.height);
  metalLayer.drawableSize = drawableSize;

  NSLog(@"view.bounds: %@", NSStringFromCGRect(self.view.bounds));
  NSLog(@"layer.bounds: %@", NSStringFromCGRect(metalLayer.bounds));
  NSLog(@"drawableSize: %@", NSStringFromCGSize(metalLayer.drawableSize));
  NSLog(@"safeAreaInsets: %@", NSStringFromUIEdgeInsets(self.view.safeAreaInsets));

  _application->onViewResize();
}

- (void)viewWillLayoutSubviews
{
  [super viewWillLayoutSubviews];
  [self NOVA_onViewResize];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
    [self NOVA_onViewResize];
  } completion:nil];
}

- (BOOL)shouldAutorotate
{
  return YES;
}

- (NSUInteger)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscape;
}

- (void)updateLoop:(CADisplayLink*)sender
{
  _application->update();
}

- (void)dealloc
{
  [self.displayLink invalidate];
  [super dealloc];
}
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow* window;
@end

@implementation AppDelegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
  self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
  self.window.rootViewController = [[ViewController alloc] init];
  [self.window makeKeyAndVisible];
  return YES;
}
@end

int main(int argc, char* argv[])
{
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
