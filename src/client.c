//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
// Copyright 2016 Samsung Electronics France SAS All Rights Reserved.
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#define HAVE_UNISTD_H
#define HAVE_SIGNAL_H
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <octypes.h>
#include <ocstack.h>

#define STR(a) #a

#define LOGf(f,p)                                    \
    printf("%s: %s=" f "\n", __FUNCTION__,STR(p), p);

static OCConnectivityType gConnectivityType = CT_DEFAULT;
static OCQualityOfService gQos = OC_HIGH_QOS;
static const char * gUri = "/light/1";

int gQuitFlag = 0;
static OCDevAddr gDestination;
static char discoveryAddr[100];

typedef struct LIGHTRESOURCE
{
    OCResourceHandle handle;
    bool state;
} LightResource;

static LightResource Light;


OCStackApplicationResult handleGet(void *ctx,
                                   OCDoHandle handle,
                                   OCClientResponse *clientResponse)
{
    printf("%s: { %d\n", __FUNCTION__, Light.state);
    OCStackApplicationResult result= OC_STACK_DELETE_TRANSACTION;

    if (!clientResponse)
    {
        return result;
    }    
    OCRepPayload* payload = (OCRepPayload*)(clientResponse->payload);
    if (!payload)
    {
        return result;
    }    


    if (!OCRepPayloadGetPropBool(payload, "state", &Light.state))
    {
        printf("%s: error: %d\n", __FUNCTION__, __LINE__);
    }

    printf("%s: } %d\n", __FUNCTION__, Light.state);
    return OC_STACK_DELETE_TRANSACTION;
}


OCStackApplicationResult handlePost(void *ctx,
                                    OCDoHandle handle,
                                    OCClientResponse *clientResponse)
{
    OCStackApplicationResult result = OC_STACK_KEEP_TRANSACTION;
    LOGf("%p", clientResponse);
    return OC_STACK_DELETE_TRANSACTION;
}

OCPayload* createPayload()
{
    OCRepPayload* payload = (OCRepPayload*) OCRepPayloadCreate();

    printf("%s: %s=%p\n", __FUNCTION__, STR(payload), payload);
    if (!payload)
    {
        exit(1);
    }
    OCRepPayloadSetPropBool(payload, "state", Light.state);

    return (OCPayload*) payload;
}

// This is a function called back when a device is discovered
OCStackApplicationResult handleDiscover(void *ctx,
                                        OCDoHandle handle,
                                        OCClientResponse *clientResponse)
{
    OCStackResult result = OC_STACK_OK;

    printf("%s: %s=%p\n", __FUNCTION__, STR(clientResponse), clientResponse);
        
    if (!clientResponse)
    {
        return OC_STACK_DELETE_TRANSACTION;
    }

    printf("%s: %s=%p\n", __FUNCTION__, STR(clientResponse->devAddr.addr), clientResponse->devAddr.addr);
 
    LOGf("%d", clientResponse->sequenceNumber);

    gDestination = clientResponse->devAddr;
    gConnectivityType = clientResponse->connType;

    OCDiscoveryPayload *payload = (OCDiscoveryPayload*) clientResponse->payload;
    if (!payload)
    {
        return OC_STACK_DELETE_TRANSACTION;
    }
    OCResourcePayload *resource = (OCResourcePayload*) payload->resources;

    while (resource)
    {
        if(resource->uri)
        {
            LOGf("%s", resource->uri);
        }
        resource = resource->next;
    }
    Light.handle = handle;
    return OC_STACK_KEEP_TRANSACTION;
}


OCStackResult get()
{
    OCStackResult result = OC_STACK_OK;
    printf("%s:\n", __FUNCTION__);
    
    OCMethod method = OC_REST_GET;
    OCCallbackData getCallback = { NULL, NULL, NULL };
    getCallback.cb = handleGet;
    OCRepPayload* payload = NULL;
    
    result = OCDoResource(&Light.handle, method, gUri, &gDestination,
                          (OCPayload*) payload,
                          gConnectivityType, gQos, &getCallback, NULL,0);

    return result;
}

OCStackResult post()
{
    OCStackResult result = OC_STACK_OK;
    OCMethod method = OC_REST_POST;
    OCRepPayload* payload = createPayload();
    OCCallbackData postCallback = { NULL, NULL, NULL };
    postCallback.cb = handlePost;

    printf("%s: %d\n", __FUNCTION__, Light.state);

    result = OCDoResource(&Light.handle, method, gUri, &gDestination,
                          (OCPayload*) payload,
                          gConnectivityType, gQos, &postCallback, NULL,0);
    
    if (result != OC_STACK_OK)
    {
        LOGf("%d", method);
        LOGf("%d", result);

        return result;
    }
    sleep(5);
    return result;
}



OCStackResult loop()
{
    OCStackResult result;

    while (!gQuitFlag)
    {
        printf("iterate: %d\n", Light.state);

        result = OCProcess();
        if (result != OC_STACK_OK)
        {
            LOGf("%d (error)", result);
            return result;
        }

        post();

        get();
        
        //changing state for next iteration
        Light.state = ! Light.state;

    }
    LOGf("%d", gQuitFlag);
    return result;
}


OCStackResult setup()
{
    OCStackResult result;

    result = OCInit(NULL, 0, OC_CLIENT) ;
    if (result != OC_STACK_OK)
    {
        LOGf("%d (error)", result);
        return result;
    }

    OCCallbackData cbData = {NULL, NULL, NULL };
    cbData.cb = handleDiscover;

    char queryUri[MAX_QUERY_LENGTH] = { 0 };
    snprintf(queryUri, sizeof (queryUri), "%s", OC_RSRVD_WELL_KNOWN_URI);
    result = OCDoResource(NULL, OC_REST_DISCOVER,
                          queryUri, NULL, 0, gConnectivityType,
                          gQos, &cbData, NULL, 0);

    if (result != OC_STACK_OK)
    {
        LOGf("%d (error)", result);
    }

    return result;
}


void finish()
{

    OCStackResult result = OCStop();

    if (result != OC_STACK_OK)
    {
        LOGf("%d (error)", result);
    }
}

/* SIGINT handler: set gQuitFlag to 1 for graceful termination */
void handleSigInt(int signum)
{
    if (signum == SIGINT)
    {
        gQuitFlag = 1;
    }
}

int main()
{
    setup();

    // Break from loop with Ctrl+C
    signal(SIGINT, handleSigInt);

    loop();

    finish();

    return 0;
}

