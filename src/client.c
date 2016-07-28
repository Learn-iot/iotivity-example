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
#include <logger.h>

#define TAG ("occlient")

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

    bool state=false;
    if(OCRepPayloadGetPropBool(payload, "state", &Light.state))
    {
        printf("get: %d\n", Light.state);
    }

    return OC_STACK_DELETE_TRANSACTION;
}


OCStackApplicationResult handlePost(void *ctx,
                                    OCDoHandle handle,
                                    OCClientResponse *clientResponse)
{
    OCStackApplicationResult result = OC_STACK_KEEP_TRANSACTION;
    OIC_LOG(INFO, TAG, "Callback Context for POST recvd successfully");

    if (clientResponse)
    {
        OIC_LOG(INFO, TAG, "=============> Post Response");
    }
    else
    {
        OIC_LOG_V(INFO, TAG, "postReqCB received Null clientResponse");
    }
    return OC_STACK_DELETE_TRANSACTION;
}

OCPayload* createPayload()
{
    OCRepPayload* payload = (OCRepPayload*) OCRepPayloadCreate();

    if (!payload)
    {
        OIC_LOG(INFO,TAG, "Failed to create put payload object");
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

    OIC_LOG(INFO,TAG,"Callback Context for DISCOVER query recvd successfully");

    if (!clientResponse)
    {
        OIC_LOG(ERROR,TAG,"Payload is NULL, No resources found");
        return OC_STACK_DELETE_TRANSACTION;
    }

    OIC_LOG_V(INFO, TAG,
              "Device =============> Discovered @ %s:%d",
              clientResponse->devAddr.addr,
              clientResponse->devAddr.port);

    OIC_LOG_V(INFO, TAG, "SEQUENCE NUMBER: %d", clientResponse->sequenceNumber);
    OIC_LOG(INFO, TAG, "=============> Get Response");

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
            OIC_LOG_V(INFO, TAG, "uri: %s", resource->uri);
        }
        resource = resource->next;
    }
    Light.handle = handle;
    return OC_STACK_KEEP_TRANSACTION;
}


OCStackResult get()
{
    OCStackResult result = OC_STACK_OK;
    OCMethod method = OC_REST_GET;
    OCCallbackData getCallback = { NULL, NULL, NULL };
    getCallback.cb = handleGet;
    OCRepPayload* payload = NULL;
    
    OIC_LOG_V(INFO,TAG,"loop: about to %d",method);
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
    
    OIC_LOG_V(INFO,TAG,"loop: about to %d",method);
    result = OCDoResource(&Light.handle, method, gUri, &gDestination,
                          (OCPayload*) payload,
                          gConnectivityType, gQos, &postCallback, NULL,0);
    
    if (result != OC_STACK_OK)
    {
        OIC_LOG_V(ERROR, TAG, "OCDoResource returns error %d with method %d", result, method);
        return result;
    }
    sleep(1);
    return result;
}



OCStackResult loop()
{
    OCStackResult result;
    OIC_LOG(INFO, TAG, "Entering occlient main loop...");

    while (!gQuitFlag)
    {
        printf("iterate: %d\n", Light.state);

        result = OCProcess();
        if (result != OC_STACK_OK)
        {
            OIC_LOG(ERROR, TAG, "OCStack process error");
            return 0;
        }

        post();
        get();
        
        //changing state for next iteration
        Light.state = ! Light.state; 
    }
    OIC_LOG(INFO, TAG, "Exiting occlient main loop...");
    return result;
}


OCStackResult setup()
{
    OCStackResult result;

    result = OCInit(NULL, 0, OC_CLIENT) ;
    if (result != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack init error");
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
        OIC_LOG(ERROR, TAG, "OCStack resource error");
    }

    return result;
}


void finish()
{
    if (OCStop() != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack stop error");
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

