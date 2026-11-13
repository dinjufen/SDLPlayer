#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "platform/Platform.h"

#include <string>
#include <vector>

namespace platform {

std::optional<std::string> ShowOpenMediaFileDialog() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setCanCreateDirectories:NO];
        [panel setTitle:@"Open media"];
        NSMutableArray<UTType*>* types = [NSMutableArray array];
        for (NSString* extension in @[@"mp4", @"m4v", @"mkv", @"mov", @"avi", @"flv", @"mp3", @"aac", @"wav", @"m3u8"]) {
            UTType* type = [UTType typeWithFilenameExtension:extension];
            if (type != nil) {
                [types addObject:type];
            }
        }
        [panel setAllowedContentTypes:types];

        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return std::nullopt;
        }

        NSURL* url = [[panel URLs] firstObject];
        if (url == nil || [url path] == nil) {
            return std::nullopt;
        }

        return std::string([[url path] UTF8String]);
    }
}

TTF_Font* OpenUiFont(const int pointSize) {
    const std::vector<std::string> candidates = {
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/System/Library/Fonts/Supplemental/Songti.ttc",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/SFNSText.ttf",
        "/System/Library/Fonts/LucidaGrande.ttc",
    };

    for (const std::string& candidate : candidates) {
        if (TTF_Font* font = TTF_OpenFont(candidate.c_str(), pointSize); font != nullptr) {
            return font;
        }
    }

    return nullptr;
}

std::optional<std::string> CommandLineMediaPath(int argc, char* argv[]) {
    if (argc > 1 && argv != nullptr && argv[1] != nullptr && argv[1][0] != '\0') {
        return std::string(argv[1]);
    }

    return std::nullopt;
}

}  // namespace platform
