//
//  PAttributedString.h
//  Pointer
//
//  Created by Yu Li on 2018-12-08.
//  Copyright © 2018 Yu Li. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <docviewer/docviewer.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSMutableAttributedString(Pointer)
- (void)highlight:(RangeSet)ranges;
@end

NS_ASSUME_NONNULL_END