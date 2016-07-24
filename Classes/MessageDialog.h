//
//  MessageDialog.h
//  SpaceExplorer
//
//  Created by João Baptista on 22/04/15.
//
//

#ifndef __SpaceExplorer__MessageDialog__
#define __SpaceExplorer__MessageDialog__

#include <string>
#include <functional>
#include "cocos2d.h"

void presentMessage(std::string message, std::string title, std::string confirmCaption, std::string cancelCaption, std::function<void()> confirmCallback, std::function<void()> cancelCallback);

#endif /* defined(__SpaceExplorer__MessageDialog__) */
