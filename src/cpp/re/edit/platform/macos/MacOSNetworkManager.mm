#include "MacOSNetworkManager.h"
#import <Foundation/Foundation.h>

namespace re::edit {

//------------------------------------------------------------------------
// MacOSNetworkManager::HttpGet
//------------------------------------------------------------------------
std::optional<std::string> MacOSNetworkManager::HttpGet(std::string const &iUrl,
                                                        std::map<std::string, std::string> const &iHeaders) const
{
  @autoreleasepool
  {

    NSURLSession *sharedSession = [NSURLSession sharedSession];

    NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:[NSURL URLWithString:[NSString stringWithUTF8String:iUrl.c_str()]]];

    for(auto &[header, value]: iHeaders)
    {
      [request setValue:[NSString stringWithUTF8String:value.c_str()] forHTTPHeaderField:[NSString stringWithUTF8String:header.c_str()]];
    }

    std::promise<std::optional<std::string>> promise{};

    auto cb = [&promise](std::optional<std::string> result) { promise.set_value(std::move(result)); };

    std::string foo{};

    NSURLSessionDataTask *dataTask = [sharedSession dataTaskWithRequest:request
                                                      completionHandler:^(NSData *data, NSURLResponse *response,
                                                                          NSError *error) {
                                                        if(error == nil)
                                                        {
                                                          NSString *text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                                                          auto result = std::string([text UTF8String]);
                                                          cb(result);
                                                        }
                                                        else
                                                        {
                                                          cb(std::nullopt);
                                                        }
                                                      }];
    [dataTask resume];

    return promise.get_future().get();
  }
}

}
