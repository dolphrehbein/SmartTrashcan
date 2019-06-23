//
//  ParticleNetwork.h
//  ParticleSDKPods
//
//  Created by Ido Kleinman on 9/24/18.
//  Copyright © 2018 Particle Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ParticleDevice.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, ParticleNetworkType) {
    ParticleNetworkTypeMicroWifi=0,
    ParticleNetworkTypeMicroCellular,
    ParticleNetworkTypeHighAvailability,
    ParticleNetworkTypeLargeSite
};


 // for now API will only return confirmed networks
typedef NS_ENUM(NSInteger, ParticleNetworkState) {
    ParticleNetworkStatePending=0,
    ParticleNetworkStateConfirmed
};

@interface ParticleNetwork : NSObject


extern NSString *const kParticleAPIBaseURL;

@property (nonatomic, strong, readonly) NSString* id;
@property (nonatomic, strong, readonly) NSString* name;
@property (nonatomic, readonly) ParticleNetworkType type;
@property (nonatomic, strong, nullable, readonly) NSString* panId;
@property (nonatomic, strong, nullable, readonly) NSString* xpanId;
@property (nonatomic, readonly) NSUInteger channel;
@property (nonatomic, readonly) NSUInteger deviceCount;
@property (nonatomic, readonly) NSUInteger gatewayCount;
@property (nonatomic, strong, nullable, readonly) NSDate* lastHeard;
@property (nonatomic, nullable) NSString* notes;
@property (nonatomic, readonly) ParticleNetworkState state;


-(nullable instancetype)initWithParams:(NSDictionary *)params NS_DESIGNATED_INITIALIZER;
-(instancetype)init __attribute__((unavailable("Must use initWithParams:")));
-(NSURLSessionDataTask *)refresh:(nullable ParticleCompletionBlock)completion;


@end

NS_ASSUME_NONNULL_END
