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
#include <stdbool.h>

#include <ocstack.h>


#define STR(a) #a

#define LOGf(f,p)                                       \
    printf("%s: %s=" f "\n", __FUNCTION__,STR(p), p);


int setup();

int gQuitFlag = 0;
static const char * gUri = "/light/1";

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
    OCEntityHandlerResult result = OC_EH_OK;
    OCEntityHandlerResponse response = {0};

    LOGf("%p", entityHandlerRequest);

    OCRepPayload *payload = (OCRepPayload *) OCRepPayloadCreate();
    if (!payload)
    {
        LOGf("%p (error)", payload);
        return OC_EH_ERROR;
    }

    if (entityHandlerRequest && (flag & OC_REQUEST_FLAG))
    {
        LOGf("%d (error)", flag);

        if (OC_REST_GET == entityHandlerRequest->method)
        {
            OCRepPayloadSetUri(payload, gUri);
            OCRepPayloadSetPropBool(payload, "state", Light.state);
            OCRepPayloadAddResourceType(payload, "core.light");
        }
        else if (OC_REST_POST == entityHandlerRequest->method)
        {

            Light.state = !Light.state;
            OCRepPayloadSetUri(payload, gUri);
            OCRepPayloadSetPropBool(payload, "state", Light.state);
            OCRepPayloadAddResourceType(payload, "core.light");
            //OCRepPayloadAddInterface(payload, DEFAULT_INTERFACE);
        }

        {
            // Format the response.  Note this requires some info about the request
            response.requestHandle = entityHandlerRequest->requestHandle;
            response.resourceHandle = entityHandlerRequest->resource;
            response.ehResult = result;
            response.payload = (OCPayload *) payload;
            response.numSendVendorSpecificHeaderOptions = 0;
            memset(response.sendVendorSpecificHeaderOptions, 0,
                   sizeof response.sendVendorSpecificHeaderOptions);
            memset(response.resourceUri, 0, sizeof response.resourceUri);
            // Indicates that response is NOT in a persistent buffer
            response.persistentBufferFlag = 0;

            // Send the response
            result = OCDoResponse(&response);
            if (result != OC_STACK_OK)
            {
                LOGf("%d (error)", result);
                result = OC_EH_ERROR;
            }
        }
    }
    OCRepPayloadDestroy(payload);
    return result;
}


OCStackResult createLightResource()
{
    Light.state = false;
    OCStackResult result = OCCreateResource(&Light.handle,
                                            "core.light",
                                            OC_RSRVD_INTERFACE_DEFAULT,
                                            gUri,
                                            handleOCEntity,
                                            NULL,
                                            OC_DISCOVERABLE | OC_OBSERVABLE);
    
    LOGf("%d", result);
    return result;
}


int loop()
{
    while (!gQuitFlag)
    {
        printf("iterate: %d\n",Light.state);
        OCStackResult result = OCProcess();
        if (result != OC_STACK_OK)
        {

            LOGf("%d (error)", result);
            return 1;
        }

        sleep(1);
    }
    LOGf("%d (error)", gQuitFlag );

    OCStackResult result = OCStop();
    if ( result != OC_STACK_OK) 
    {
        return 2;
    }
}

int setup()
{
    OCStackResult result = OCInit(NULL, 0, OC_SERVER);
    if ( result != OC_STACK_OK)
    {
        LOGf("%d (error)", result);
        return 1;
    }

    result = createLightResource();

    if ( result != OC_STACK_OK)
    {
        LOGf("%d (error)", result);
        return 2;
    }
}
