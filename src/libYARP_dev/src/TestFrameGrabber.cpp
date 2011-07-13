// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2008 RobotCub Consortium
 * Authors: Paul Fitzpatrick
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 *
 */


#include <yarp/dev/TestFrameGrabber.h>
#include <yarp/dev/PolyDriver.h>

#include <yarp/sig/ImageDraw.h>
#include <yarp/os/Random.h>

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::sig::draw;

#define VOCAB_BALL VOCAB4('b','a','l','l')
#define VOCAB_GRID VOCAB4('g','r','i','d')
#define VOCAB_RAND VOCAB4('r','a','n','d')


void TestFrameGrabber::createTestImage(yarp::sig::ImageOf<yarp::sig::PixelRgb>&
                                       image) {
    image.resize(w,h);
    image.zero();
    switch (mode) {
    case VOCAB_BALL:
        {
            addCircle(image,PixelRgb(0,255,0),bx,by,15);
            addCircle(image,PixelRgb(0,255,255),bx,by,8);
            if (ct%5!=0) {
                rnd *= 65537;
                rnd += 17;
                bx += (rnd%5)-2;
                rnd *= 65537;
                rnd += 17;
                by += (rnd%5)-2;
            } else {
                int dx = w/2 - bx;
                int dy = h/2 - by;
                if (dx>0) { bx++; }
                if (dx<0) { bx--; }
                if (dy>0) { by++; }
                if (dy<0) { by--; }
            }
        }
        break;
    case VOCAB_GRID:
        {
            int ww = image.width();
            int hh = image.height();
            if (ww>1&&hh>1) {
                for (int x=0; x<ww; x++) {
                    for (int y=0; y<hh; y++) {
                        double xx = ((double)x)/(ww-1);
                        double yy = ((double)y)/(hh-1);
                        int r = int(0.5+255*xx);
                        int g = int(0.5+255*yy);
                        bool act = (y==ct);
                        image.pixel(x,y) = PixelRgb(r,g,act*255);
                    }
                }
            }
        }
        break;
    case VOCAB_LINE:
    default:
        {
            for (int i=0; i<image.width(); i++) {
                image.pixel(i,ct).r = 255;
            }
        }
        break;
    case VOCAB_RAND:
        {
            // from Alessandro Scalzo

            static unsigned char r=128,g=128,b=128;
            
            int ww = image.width();
            int hh = image.height();
            
            if (ww>1&&hh>1) {
                for (int x=0; x<ww; x++) {
                    for (int y=0; y<hh; y++) {
                        //r+=(rand()%3)-1;
                        //g+=(rand()%3)-1;
                        //b+=(rand()%3)-1;
                        r += Random::uniform(-1,1);
                        g += Random::uniform(-1,1);
                        b += Random::uniform(-1,1);
                        image.pixel(x,y) = PixelRgb(r,g,b);
                    }
                }
            }
        }
        break;
    }
    ct++;
    if (ct>=image.height()) {
        ct = 0;
    }
    if (by>=image.height()) {
        by = image.height()-1;
    }
    if (bx>=image.width()) {
        bx = image.width()-1;
    }
    if (bx<0) bx = 0;
    if (by<0) by = 0;
}

