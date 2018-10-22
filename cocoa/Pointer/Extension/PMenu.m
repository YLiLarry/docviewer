//
//  PMenu.m
//  Pointer
//
//  Created by Yu Li on 2018-10-22.
//  Copyright © 2018 Yu Li. All rights reserved.
//

#import "PMenu.h"

@implementation PMenu

@end

@implementation NSMenu(Pointer)
- (void)filterMenuItems
{
    NSArray* itemarray = self.itemArray;
    for (int i = itemarray.count - 1; i >= 0; i--) {
        NSMenuItem* item = itemarray[i];
        if (! [item.title containsString:@"Copy"]
            && ! [item.title containsString:@"Cut"]
            && ! [item.title containsString:@"Paste"]
            && ! [item.title containsString:@"Reload"]
            && ! [item.title containsString:@"Look Up"])
        {
            [self removeItem:item];
        }
    }
}
@end
