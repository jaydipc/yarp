// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2010 RobotCub Consortium
 * Authors: Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */

#include <stdio.h>
#include <yarp/os/all.h>

class RosLookup {
public:
    bool valid;
    yarp::os::ConstString hostname;
    int portnum;
    yarp::os::ConstString protocol;
    bool verbose;

    RosLookup(bool verbose) : verbose(verbose) {
        valid = false;
    }

    bool lookupCore(const char *name);

    bool lookupTopic(const char *name);

    yarp::os::ConstString toString() {
        char buf[1000];
        sprintf(buf,"/%s:%d/", hostname.c_str(), portnum);
        return buf;
    }

    yarp::os::Contact toContact(const char *carrier) {
        return yarp::os::Contact::bySocket(carrier,hostname.c_str(), portnum);
    }

    static yarp::os::Contact getRosCoreAddressFromEnv();
    static yarp::os::Contact getRosCoreAddress();
};
