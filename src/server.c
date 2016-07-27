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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ocstack.h>
#include <logger.h>

int gQuitFlag = 0;

#define TAG ("ocserver")

#define STR(a) #a

typedef struct LIGHTRESOURCE
{
    OCResourceHandle handle;
    bool state;
} LightResource;

static LightResource Light;


#ifdef __linux__
#include "server/platform/linux/platform.c"
#elif defined ARDUINO
#include "server/platform/arduino/platform.c"
#else
#include "platform/default/platform.c"
#warning "TODO: platform may not supported"
#endif


OCStackResult createLightResource();



OCEntityHandlerResult handleOCEntity(OCEntityHandlerFlag flag,
                                     OCEntityHandlerRequest *entityHandlerRequest,
                                     void *callbackParam)
{
    OCEntityHandlerResult ehRet = OC_EH_OK;
    OCEntityHandlerResponse response = {0};

    OIC_LOG_V(INFO, TAG, "%s",__FUNCTION__);

    OCRepPayload *payload = (OCRepPayload *) OCRepPayloadCreate();
    if (!payload)
    {
        OIC_LOG(ERROR, TAG, "Failed to allocate Payload");
        return OC_EH_ERROR;
    }

    if (entityHandlerRequest && (flag & OC_REQUEST_FLAG))
    {
        OIC_LOG(INFO, TAG, "Flag includes OC_REQUEST_FLAG");

        if (OC_REST_GET == entityHandlerRequest->method)
        {
            OCRepPayloadSetUri(payload, "/a/light");
            OCRepPayloadSetPropBool(payload, "state", Light.state);
            OCRepPayloadAddResourceType(payload, "core.light");
        }
        else if (OC_REST_POST == entityHandlerRequest->method)
        {

            Light.state = !Light.state;
            OCRepPayloadSetUri(payload, "/a/light");
            OCRepPayloadSetPropBool(payload, "state", Light.state);
            OCRepPayloadAddResourceType(payload, "core.light");
            //OCRepPayloadAddInterface(payload, DEFAULT_INTERFACE);
        }

        if (ehRet == OC_EH_OK)
        {
            // Format the response.  Note this requires some info about the request
            response.requestHandle = entityHandlerRequest->requestHandle;
            response.resourceHandle = entityHandlerRequest->resource;
            response.ehResult = ehRet;
            response.payload = (OCPayload *) payload;
            response.numSendVendorSpecificHeaderOptions = 0;
            memset(response.sendVendorSpecificHeaderOptions, 0,
                   sizeof response.sendVendorSpecificHeaderOptions);
            memset(response.resourceUri, 0, sizeof response.resourceUri);
            // Indicates that response is NOT in a persistent buffer
            response.persistentBufferFlag = 0;

            // Send the response
            if (OCDoResponse(&response) != OC_STACK_OK)
            {
                OIC_LOG(ERROR, TAG, "Error sending response");
                ehRet = OC_EH_ERROR;
            }
        }
    }
    if (entityHandlerRequest && (flag & OC_OBSERVE_FLAG))
    {
        if (OC_OBSERVE_REGISTER == entityHandlerRequest->obsInfo.action)
        {
            OIC_LOG(INFO, TAG, "Received OC_OBSERVE_REGISTER from client");
        }
        else if (OC_OBSERVE_DEREGISTER == entityHandlerRequest->obsInfo.action)
        {
            OIC_LOG(INFO, TAG, "Received OC_OBSERVE_DEREGISTER from client");
        }
    }
    OCRepPayloadDestroy(payload);
    return ehRet;
}


OCStackResult createLightResource()
{
    Light.state = false;
    OCStackResult res = OCCreateResource(&Light.handle,
                                         "core.light",
                                         OC_RSRVD_INTERFACE_DEFAULT,
                                         "/a/light",
                                         handleOCEntity,
                                         NULL,
                                         OC_DISCOVERABLE | OC_OBSERVABLE);

    OIC_LOG_V(INFO, TAG, "Created Light resource with result: %s", res);
    return res;
}


int loop()
{
    while (!gQuitFlag)
    {
        OIC_LOG_V(INFO, TAG, "iterate: %d",Light.state);
        if (OCProcess() != OC_STACK_OK)
        {
            OIC_LOG(ERROR, TAG, "OCStack process error");
            return 0;
        }

        sleep(1);
    }
    OIC_LOG(INFO, TAG, "Exiting ocserver main loop...");
}

int setup()
{
    if (OCInit(NULL, 0, OC_SERVER) != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack init error");
        return -1;
    }

    /*
     * Declare and create the example resource: Light
     */
    if (createLightResource() != OC_STACK_OK)
    {
        OIC_LOG(ERROR, TAG, "OCStack cannot create resource...");
    }

}
