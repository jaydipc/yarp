// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2006 Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#ifndef _YARP2_NAME_
#define _YARP2_NAME_

#include <yarp/os/impl/String.h>
#include <yarp/os/impl/Address.h>

namespace yarp {
    namespace os {
        namespace impl {
            class Name;
        }
    }
}

/**
 * Simple abstraction for a YARP port name.
 */
class yarp::os::impl::Name {
public:

    /** 
     * Constructor.
     * 
     * @param txt the port name.
     */
    Name(const String& txt);

    /** 
     * Check if port name begins with "/"
     * 
     * 
     * @return true if port name begins with "/"
     */
    bool isRooted() const;

    /** 
     * Create an address from the name.  Fills in carrier and name fields.
     * Deals with "tcp://port/name" style syntax.
     * 
     * @return a partially filled address (there is no communication with
     * the name server, just parsing of information in the name).
     */
    Address toAddress() const;

private:
    String txt;
};

#endif
