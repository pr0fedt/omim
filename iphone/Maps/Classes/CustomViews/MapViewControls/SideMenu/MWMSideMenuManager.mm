//
//  MWMSideMenuManager.m
//  Maps
//
//  Created by Ilya Grechuhin on 24.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "BookmarksRootVC.h"
#import "Framework.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMMapViewControlsCommon.h"
#import "MWMMapViewControlsManager.h"
#import "MWMSideMenuButton.h"
#import "MWMSideMenuButtonDelegate.h"
#import "MWMSideMenuDelegate.h"
#import "MWMSideMenuDownloadBadge.h"
#import "MWMSideMenuManager.h"
#import "MWMSideMenuView.h"
#import "SettingsAndMoreVC.h"
#import "ShareActionSheet.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "map/information_display.hpp"

static NSString * const kMWMSideMenuViewsNibName = @"MWMSideMenuViews";

extern NSString * const kAlohalyticsTapEventKey;

@interface MWMSideMenuManager() <MWMSideMenuInformationDisplayProtocol, MWMSideMenuTapProtocol>

@property (weak, nonatomic) MapViewController * controller;
@property (nonatomic) IBOutlet MWMSideMenuButton * menuButton;
@property (nonatomic) IBOutlet MWMSideMenuView * sideMenu;
@property (nonatomic) IBOutlet MWMSideMenuDownloadBadge * downloadBadge;

@end

@implementation MWMSideMenuManager

- (instancetype)initWithParentController:(MapViewController *)controller
{
  self = [super init];
  if (self)
  {
    self.controller = controller;
    [[NSBundle mainBundle] loadNibNamed:kMWMSideMenuViewsNibName owner:self options:nil];
    [self.controller.view addSubview:self.menuButton];
    [self.menuButton setup];
    self.menuButton.delegate = self;
    self.sideMenu.delegate = self;
    [self addCloseMenuWithTap];
    self.state = MWMSideMenuStateInactive;
  }
  return self;
}

- (void)addCloseMenuWithTap
{
  UITapGestureRecognizer * const tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggleMenu)];
  [self.sideMenu.dimBackground addGestureRecognizer:tap];
}

#pragma mark - Actions

- (IBAction)menuActionOpenBookmarks
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"bookmarks"];
  BookmarksRootVC * const vc = [[BookmarksRootVC alloc] init];
  [self.controller.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionDownloadMaps
{
  [self.controller pushDownloadMaps];
}

- (IBAction)menuActionOpenSettings
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"settingsAndMore"];
  SettingsAndMoreVC * const vc = [[SettingsAndMoreVC alloc] initWithStyle:UITableViewStyleGrouped];
  [self.controller.navigationController pushViewController:vc animated:YES];
}

- (IBAction)menuActionShareLocation
{
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"share@"];
  CLLocation const * const location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (!location)
  {
    [[[UIAlertView alloc] initWithTitle:L(@"unknown_current_position") message:nil delegate:nil cancelButtonTitle:L(@"ok") otherButtonTitles:nil] show];
    return;
  }
  CLLocationCoordinate2D const coord = location.coordinate;
  ShareInfo * const info = [[ShareInfo alloc] initWithText:nil lat:coord.latitude lon:coord.longitude myPosition:YES];
  self.controller.shareActionSheet = [[ShareActionSheet alloc] initWithInfo:info viewController:self.controller];
  UIView const * const parentView = self.controller.view;
  [self.controller.shareActionSheet showFromRect:CGRectMake(parentView.midX, parentView.height - 40.0, 0.0, 0.0)];
}

- (IBAction)menuActionOpenSearch
{
  self.controller.controlsManager.hidden = YES;
  [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"search"];
  [self.controller.searchView setState:SearchViewStateFullscreen animated:YES];
}

- (void)toggleMenu
{
  if (self.state == MWMSideMenuStateActive)
    self.state = MWMSideMenuStateInactive;
  else if (self.state == MWMSideMenuStateInactive)
    self.state = MWMSideMenuStateActive;
}

- (void)handleSingleTap
{
  [self toggleMenu];
}

- (void)handleDoubleTap
{
  [self menuActionOpenSearch];
}

#pragma mark - MWMSideMenuInformationDisplayProtocol

- (void)setRulerPivot:(m2::PointD)pivot
{
  // Workaround for old ios when layoutSubviews are called in undefined order.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    GetFramework().GetInformationDisplay().SetWidgetPivot(InformationDisplay::WidgetType::Ruler, pivot);
  });
}

- (void)setCopyrightLabelPivot:(m2::PointD)pivot
{
  // Workaround for old ios when layoutSubviews are called in undefined order.
  dispatch_async(dispatch_get_main_queue(), ^
  {
    GetFramework().GetInformationDisplay().SetWidgetPivot(InformationDisplay::WidgetType::CopyrightLabel, pivot);
  });
}

- (void)showMenu
{
  self.menuButton.alpha = 1.0;
  self.sideMenu.alpha = 0.0;
  [self.controller.view addSubview:self.sideMenu];
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.menuButton.alpha = 0.0;
    self.sideMenu.alpha = 1.0;
  }
  completion:^(BOOL finished)
  {
    [self.menuButton setHidden:YES animated:NO];
  }];
}

- (void)hideMenu
{
  [UIView animateWithDuration:framesDuration(3) animations:^
  {
    self.menuButton.alpha = 1.0;
    self.sideMenu.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    [self.sideMenu removeFromSuperview];
  }];
}

- (void)addDownloadBadgeToView:(UIView<MWMSideMenuDownloadBadgeOwner> *)view
{
  int const count = GetFramework().GetCountryTree().GetActiveMapLayout().GetOutOfDateCount();
  if (count > 0)
  {
    self.downloadBadge.outOfDateCount = count;
    view.downloadBadge = self.downloadBadge;
    [self.downloadBadge showAnimatedAfterDelay:framesDuration(10)];
  }
}

#pragma mark - Properties

- (void)setState:(MWMSideMenuState)state
{
  if (_state == state)
    return;
  [self.downloadBadge hide];
  switch (state)
  {
    case MWMSideMenuStateActive:
      [Alohalytics logEvent:kAlohalyticsTapEventKey withValue:@"menu"];
      [self addDownloadBadgeToView:self.sideMenu];
      [self showMenu];
      [self.sideMenu setup];
      break;
    case MWMSideMenuStateInactive:
      [self addDownloadBadgeToView:self.menuButton];
      if (_state == MWMSideMenuStateActive)
      {
        [self.menuButton setHidden:NO animated:NO];
        [self hideMenu];
      }
      else
      {
        [self.menuButton setHidden:NO animated:YES];
      }
      break;
    case MWMSideMenuStateHidden:
      [self.menuButton setHidden:YES animated:YES];
      [self hideMenu];
      break;
  }
  _state = state;
  [self.controller updateStatusBarStyle];
}

@end
