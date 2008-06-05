// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2006 Giorgio Metta
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#include <ace/config.h>
#include <ace/OS.h>
#include <ace/Log_Msg.h>
#include <yarp/os/PortablePair.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Time.h>
#include <yarp/os/Network.h>
#include <yarp/os/Thread.h>
#include <yarp/os/Vocab.h>
#include <yarp/os/Stamp.h>
#include <yarp/String.h>
#include <yarp/NetType.h>

#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/ControlBoardInterfacesImpl.h>
#include <yarp/dev/ControlBoardHelpers.h>

#include <yarp/sig/Vector.h>

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;

namespace yarp{
    namespace dev {
        class RemoteControlBoard;
    }
}

/**
 * @ingroup dev_impl_wrapper
 *
 * The client side of the control board, connects to a ServerControlBoard.
 */
class yarp::dev::RemoteControlBoard : 
    public IPidControl,
            public IPositionControl, 
            public IVelocityControl,
            public IEncoders,
            public IAmplifierControl,
            public IControlLimits,
            public IAxisInfo,
            public IControlCalibration2,
			public DeviceDriver {

protected:
    Port rpc_p;
    Port command_p;
    Port state_p;

    PortReaderBuffer<yarp::sig::Vector> state_buffer;
    PortWriterBuffer<CommandMessage> command_buffer;

    String remote;
    String local;
    int nj;

    /** 
     * Send a SET command without parameters and wait for a reply.
     * @param code is the command Vocab identifier.
     * @return true/false on success/failure.
     */
    bool setCommand(int code) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(code);

        bool ok = rpc_p.write(cmd, response);
        return CHECK_FAIL(ok, response);
    }



    /** 

     * Send a SET command and an additional double valued variable 

     * and then wait for a reply.

     * @param code is the command to send.

     * @param v is a double valued parameter.

     * @return true/false on success/failure.

     */

    bool setCommand(int code, double v) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(code);

        cmd.addDouble(v);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /**

     * Send a SET command with an additional integer valued variable

     * and then wait for a reply.

     * @param code is the command to send.

     * @param v is an integer valued parameter.

     * @return true/false on success/failure.

     */

    bool setCommand(int code, int v) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(code);

        cmd.addInt(v);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /**

     * Send a GET command expecting a double value in return.

     * @param code is the Vocab code of the GET command.

     * @param v is a reference to the return variable.

     * @return true/false on success/failure.

     */

    bool getCommand(int code, double& v) const {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(code);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            // response should be [cmd] [name] value

            v = response.get(2).asDouble();

            return true;

        }

        return false;

    }



    /**

     * Send a GET command expecting an integer value in return.

     * @param code is the Vocab code of the GET command.

     * @param v is a reference to the return variable.

     * @return true/false on success/failure.

     */

    bool getCommand(int code, int& v) const {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(code);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            // response should be [cmd] [name] value

            v = response.get(2).asInt();

            return true;

        }

        return false;

    }



    /**

     * Helper method to set a double value to a single axis.

     * @param code is the name of the command to be transmitted

     * @param j is the axis

     * @param val is the double value

     * @return true/false on success/failure

     */

    bool setDouble (int code, int j, double val) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(code);

        cmd.addInt(j);

        cmd.addDouble(val);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /** 

     * Helper method used to set an array of double to all axes.

     * @param v is the command to set

     * @param val is the double array (of length nj)

     * @return true/false on success/failure

     */

    bool setDoubleArray(int v, const double *val) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(v);

        Bottle& l = cmd.addList();

        int i;

        for (i = 0; i < nj; i++)

            l.addDouble(val[i]);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /**

     * Helper method used to get a double value from the remote peer.

     * @param v is the command to query for

     * @param j is the axis number

     * @param val is the return value

     * @return true/false on success/failure

     */

    bool getDouble(int v, int j, double *val) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(v);

        cmd.addInt(j);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            // ok

            *val = response.get(2).asDouble();

            return true;

        }

        return false;

    }



    /**

     * Helper method to get an array of double from the remote peer.

     * @param v is the name of the command

     * @param val is the array of double

     * @return true/false on success/failure

     */

    bool getDoubleArray(int v, double *val) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(v);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            int i;

            Bottle& l = *(response.get(2).asList());

            if (&l == 0)

                return false;



            int njs = l.size();

            ACE_ASSERT (nj == njs);

            for (i = 0; i < nj; i++)

                val[i] = l.get(i).asDouble();

            return true;

        }



        return false;

    }



public:

    /**

     * Constructor.

     */

    RemoteControlBoard() { 

        nj = 0;

    }



    /**

     * Destructor.

     */

    virtual ~RemoteControlBoard() {

    }



    /**

     * Default open.

     * @return always true.

     */

    virtual bool open() {

        return true;

    }



    virtual bool open(Searchable& config) {

        remote = config.find("remote").asString().c_str();

        local = config.find("local").asString().c_str();

        ConstString carrier = 

            config.check("carrier",

                         Value("tcp"),

                         "default carrier for streaming robot state").asString().c_str();

        if (local != "") {

            String s1 = local;

            s1 += "/rpc:o";

            rpc_p.open(s1.c_str());

            s1 = local;

            s1 += "/command:o";

            command_p.open(s1.c_str());

            s1 = local;

            s1 += "/state:i";

            state_p.open(s1.c_str());

        }



        bool connectionProblem = false;

        if (remote != "") {

            String s1 = remote;

            s1 += "/rpc:i";

            String s2 = local;

            s2 += "/rpc:o";

            bool ok = Network::connect(s2.c_str(), s1.c_str());

            if (!ok) {

                printf("Problem connecting to %s, is the remote device available?\n", s1.c_str());

                connectionProblem = true;

            }

            s1 = remote;

            s1 += "/command:i";

            s2 = local;

            s2 += "/command:o";

            ok = Network::connect(s2.c_str(), s1.c_str(), carrier);

            if (!ok) {

                printf("Problem connecting to %s, is the remote device available?\n", s1.c_str());

                connectionProblem = true;

            }

            s1 = remote;

            s1 += "/state:o";

            s2 = local;

            s2 += "/state:i";

            ok = Network::connect(s1.c_str(), s2.c_str(), carrier);

            if (!ok) {

                printf("Problem connecting to %s, is the remote device available?\n", s1.c_str());

                connectionProblem = true;

            }

        }



        if (connectionProblem) {

            return false;

        }



        state_buffer.attach(state_p);

		state_buffer.setStrict(false);

        command_buffer.attach(command_p);



        bool ok = getCommand(VOCAB_AXES, nj);

        if (nj==0) {
            ok = false;
        }

        if (!ok) {
            ACE_OS::printf("Problems with obtaining the number of controlled axes\n");
            return false;
        }

        return true;
    }

    /**
     * Close the device driver and stop the port connections.
     * @return true/false on success/failure.
     */
    virtual bool close() {
        rpc_p.close();
        command_p.close();
        state_p.close();
        return true;
    }

    /** 
     * Set new pid value for a joint axis.
     * @param j joint number
     * @param pid new pid value
     * @return true/false on success/failure
     */

    virtual bool setPid(int j, const Pid &pid) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_SET);
        cmd.addVocab(VOCAB_PID);
        cmd.addInt(j);
        Bottle& l = cmd.addList();
        l.addDouble(pid.kp);
        l.addDouble(pid.kd);
        l.addDouble(pid.ki);
        l.addDouble(pid.max_int);
        l.addDouble(pid.max_output);
        l.addDouble(pid.offset);
        l.addDouble(pid.scale);
        bool ok = rpc_p.write(cmd, response);
        return CHECK_FAIL(ok, response);
    }

    /** 
     * Set new pid value on multiple axes.
     * @param pids pointer to a vector of pids
     * @return true/false upon success/failure
     */
    virtual bool setPids(const Pid *pids) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_SET);
        cmd.addVocab(VOCAB_PIDS);
        Bottle& l = cmd.addList();
        int i;
        for (i = 0; i < nj; i++) {
            Bottle& m = l.addList();
            m.addDouble(pids[i].kp);
            m.addDouble(pids[i].kd);
            m.addDouble(pids[i].ki);
            m.addDouble(pids[i].max_int);
            m.addDouble(pids[i].max_output);
            m.addDouble(pids[i].offset);
            m.addDouble(pids[i].scale);
        }

        bool ok = rpc_p.write(cmd, response);
        return CHECK_FAIL(ok, response);
    }


    /** 
     * Set the controller reference point for a given axis.
     * Warning this method can result in very large torques 
     * and should be used carefully. If you do not understand
     * this warning you should avoid using this method. 
     * Have a look at other interfaces (e.g. position control).
     * @param j joint number
     * @param ref new reference point
     * @return true/false upon success/failure
     */
    virtual bool setReference(int j, double ref) {
        return setDouble(VOCAB_REF, j, ref);
    }


    /** 
     * Set the controller reference points, multiple axes.
     * Warning this method can result in very large torques 
     * and should be used carefully. If you do not understand
     * this warning you should avoid using this method. 
     * Have a look at other interfaces (e.g. position control).
     * @param refs pointer to the vector that contains the new reference points.
     * @return true/false upon success/failure
     */
    virtual bool setReferences(const double *refs) {
        return setDoubleArray(VOCAB_REFS, refs);
    }


    /** 
     * Set the error limit for the controller on a specifi joint
     * @param j joint number
     * @param limit limit value
     * @return true/false on success/failure
     */
    virtual bool setErrorLimit(int j, double limit) {
        return setDouble(VOCAB_LIM, j, limit);
    }

    /** 
     * Get the error limit for the controller on all joints.
     * @param limits pointer to the vector with the new limits
     * @return true/false on success/failure
     */
    virtual bool setErrorLimits(const double *limits) {
        return setDoubleArray(VOCAB_LIMS, limits);
    }

    /** 
     * Get the current error for a joint.
     * @param j joint number
     * @param err pointer to the storage for the return value
     * @return true/false on success failure
     */
    virtual bool getError(int j, double *err) {
        return getDouble(VOCAB_ERR, j, err);
    }

    /** Get the error of all joints.
     * @param errs pointer to the vector that will store the errors
     */
    virtual bool getErrors(double *errs) {
        return getDoubleArray(VOCAB_ERRS, errs);
    }

    /** 
     * Get the output of the controller (e.g. pwm value)
     * @param j joint number
     * @param out pointer to storage for return value
     * @return success/failure
     */
    virtual bool getOutput(int j, double *out) {
        return getDouble(VOCAB_OUTPUT, j, out);
    }

    /** 
     * Get the output of the controllers (e.g. pwm value)
     * @param outs pinter to the vector that will store the output values
     * @return true/false on success/failure
     */
    virtual bool getOutputs(double *outs) {
        return getDoubleArray(VOCAB_OUTPUTS, outs);
    }

    /** 
     * Get current pid value for a specific joint.
     * @param j joint number
     * @param pid pointer to storage for the return value.
     * @return success/failure
     */
    virtual bool getPid(int j, Pid *pid) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_GET);
        cmd.addVocab(VOCAB_PID);
        cmd.addInt(j);
        bool ok = rpc_p.write(cmd, response);
        if (CHECK_FAIL(ok, response)) {
            Bottle& l = *(response.get(2).asList());
            if (&l == 0)
                return false;
            pid->kp = l.get(0).asDouble();
            pid->kd = l.get(1).asDouble();
            pid->ki = l.get(2).asDouble();
            pid->max_int = l.get(3).asDouble();
            pid->max_output = l.get(4).asDouble();
            pid->offset = l.get(5).asDouble();
            pid->scale = l.get(6).asDouble();
            return true;
        }
        return false;
    }

    /** 
     * Get current pid value for all controlled axes.
     * @param pids vector that will store the values of the pids.
     * @return success/failure
     */
    virtual bool getPids(Pid *pids) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_GET);
        cmd.addVocab(VOCAB_PIDS);
        bool ok = rpc_p.write(cmd, response);
        if (CHECK_FAIL(ok, response)) {
            int i;
            Bottle& l = *(response.get(2).asList());
            if (&l == 0)
                return false;
            const int njs = l.size();
            ACE_ASSERT (njs == nj);
            for (i = 0; i < nj; i++)
                {
                    Bottle& m = *(l.get(i).asList());
                    if (&m == 0)
                        return false;
                    pids->kp = m.get(0).asDouble();
                    pids->kd = m.get(1).asDouble();
                    pids->ki = m.get(2).asDouble();
                    pids->max_int = m.get(3).asDouble();
                    pids->max_output = m.get(4).asDouble();
                    pids->offset = m.get(5).asDouble();
                    pids->scale = m.get(6).asDouble();
                }
            return true;
        }
        return false;
    }

    /** 
     * Get the current reference position of the controller for a specific joint.
     * @param j is joint number
     * @param ref pointer to storage for return value
     * @return true/false on success/failure
     */
    virtual bool getReference(int j, double *ref) {
        return getDouble(VOCAB_REF, j, ref);
    }

    /** Get the current reference position of all controllers.
     * @param refs vector that will store the output.
     */
    virtual bool getReferences(double *refs) {
        return getDoubleArray(VOCAB_REFS, refs);
    }

    /** 
     * Get the error limit for the controller on a specific joint
     * @param j is the joint number
     * @param limit pointer to storage
     * @return true/false on success/failure
     */
    virtual bool getErrorLimit(int j, double *limit) {
        return getDouble(VOCAB_LIM, j, limit);
    }

    /** 
     * Get the error limit for all controllers
     * @param limits pointer to the array that will store the output
     * @return true/false on success/failure
     */
    virtual bool getErrorLimits(double *limits) {
        return getDoubleArray(VOCAB_LIMS, limits);
    }

    /** 
     * Reset the controller of a given joint, usually sets the 
     * current position of the joint as the reference value for the PID, and resets
     * the integrator.
     * @param j joint number
     * @return true on success, false on failure.
     */
    virtual bool resetPid(int j) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_SET);
        cmd.addVocab(VOCAB_RESET);
        cmd.addInt(j);
        bool ok = rpc_p.write(cmd, response);
        return CHECK_FAIL(ok, response);
    }

    /** 
     * Disable the pid computation for a joint
     * @param j is the joint number
     * @return true/false on success/failure
     */
    virtual bool disablePid(int j) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_SET);
        cmd.addVocab(VOCAB_DISABLE);
        cmd.addInt(j);
        bool ok = rpc_p.write(cmd, response);
        return CHECK_FAIL(ok, response);
    }

    /** 
     * Enable the pid computation for a joint
     * @param j is the joint number
     * @return true/false on success/failure
     */
    virtual bool enablePid(int j) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_SET);
        cmd.addVocab(VOCAB_ENABLE);
        cmd.addInt(j);
        bool ok = rpc_p.write(cmd, response);
        return CHECK_FAIL(ok, response);
    }

    /* IEncoder */

    /**
     * Reset encoder, single joint. Set the encoder value to zero 
     * @param j is the axis number
     * @return true/false on success/failure
     */
    virtual bool resetEncoder(int j) {
        Bottle cmd, response;
        cmd.addVocab(VOCAB_SET);
        cmd.addVocab(VOCAB_E_RESET);
        cmd.addInt(j);
        bool ok = rpc_p.write(cmd, response);
        return CHECK_FAIL(ok, response);
    }

    /**
     * Reset encoders. Set the encoders value to zero 
     * @return true/false
     */
    virtual bool resetEncoders() {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(VOCAB_E_RESETS);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /**

     * Set the value of the encoder for a given joint. 

     * @param j encoder number

     * @param val new value

     * @return true/false on success/failure

     */

    virtual bool setEncoder(int j, double val) {

        return setDouble(VOCAB_ENCODER, j, val);

    }



    /**

     * Set the value of all encoders.

     * @param vals pointer to the new values

     * @return true/false

     */

    virtual bool setEncoders(const double *vals) {

        return setDoubleArray(VOCAB_ENCODERS, vals);

    }



    /**

     * Read the value of an encoder.

     * @param j encoder number

     * @param v pointer to storage for the return value

     * @return true/false, upon success/failure (you knew it, uh?)

     */

    virtual bool getEncoder(int j, double *v) {

        return getDouble(VOCAB_ENCODER, j, v);

    }



    /**
     * Read the position of all axes.
     * @param encs pointer to the array that will contain the output
     * @return true/false on success/failure
     */
    virtual bool getEncoders(double *encs) {
        // this should be non blocking for several good reasons
        // for example clients should never block when the server
        // dies.
        // --nat
        Vector *v = state_buffer.read(false);
        if (v != NULL) {

            ACE_ASSERT (v->size() == nj);
            ACE_OS::memcpy (encs, &(v->operator [](0)), sizeof(double)*nj);
            return true;

        }
        return false;
    }


    /**
     * Read the istantaneous speed of an axis.
     * @param j axis number
     * @param sp pointer to storage for the output
     * @return true if successful, false ... otherwise.
     */
    virtual bool getEncoderSpeed(int j, double *sp) {
        return getDouble(VOCAB_ENCODER_SPEED, j, sp);
    }


    /**
     * Read the instantaneous speed of all axes.
     * @param spds pointer to storage for the output values
     * @return guess what? (true/false on success or failure).
     */
    virtual bool getEncoderSpeeds(double *spds) {
        return getDoubleArray(VOCAB_ENCODER_SPEEDS, spds);
    }

    /**
     * Read the instantaneous acceleration of an axis.
     * @param j axis number
     * @param acc pointer to the array that will contain the output
     */

    virtual bool getEncoderAcceleration(int j, double *acc) {
        return getDouble(VOCAB_ENCODER_ACCELERATION, j, acc);
    }

    /**
     * Read the istantaneous acceleration of all axes.
     * @param accs pointer to the array that will contain the output
     * @return true if all goes well, false if anything bad happens. 
     */
    virtual bool getEncoderAccelerations(double *accs) {

        return getDoubleArray(VOCAB_ENCODER_ACCELERATIONS, accs);

    }



    /* IPositionControl */



    /**

     * Get the number of controlled axes. This command asks the number of controlled

     * axes for the current physical interface.

     * @param ax pointer to storage

     * @return true/false.

     */

    virtual bool getAxes(int *ax) {

        return getCommand(VOCAB_AXES, *ax);

    }



    /** 

     * Set position mode. This command

     * is required by control boards implementing different

     * control methods (e.g. velocity/torque), in some cases

     * it can be left empty.

     * return true/false on success/failure

     */

    virtual bool setPositionMode() {

        return setCommand(VOCAB_POSITION_MODE);

    }



    /** 

     * Set new reference point for a single axis.

     * @param j joint number

     * @param ref specifies the new ref point

     * @return true/false on success/failure

     */

    virtual bool positionMove(int j, double ref) { 

        return setDouble(VOCAB_POSITION_MOVE, j, ref);

    }



    /** 

     * Set new reference point for all axes.

     * @param refs array, new reference points

     * @return true/false on success/failure

     */

    virtual bool positionMove(const double *refs) { 

        CommandMessage& c = command_buffer.get();

		c.head.clear();

        c.head.addVocab(VOCAB_POSITION_MOVES);

        c.body.size(nj);

        ACE_OS::memcpy(&(c.body[0]), refs, sizeof(double)*nj);

        command_buffer.write();

        return true;

    }



    /** 

     * Set relative position. The command is relative to the 

     * current position of the axis.

     * @param j joint axis number

     * @param delta relative command

     * @return true/false on success/failure

     */

    virtual bool relativeMove(int j, double delta) { 

        return setDouble(VOCAB_RELATIVE_MOVE, j, delta); 

    }



    /** 

     * Set relative position, all joints.

     * @param deltas pointer to the relative commands

     * @return true/false on success/failure

     */

    virtual bool relativeMove(const double *deltas) { 

        return setDoubleArray(VOCAB_RELATIVE_MOVES, deltas); 

    }



    /** Check if the current trajectory is terminated. Non blocking.

     * @return true if the trajectory is terminated, false otherwise

     */

    virtual bool checkMotionDone(int j, bool *flag) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(VOCAB_MOTION_DONE);

        cmd.addInt(j);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            *flag = (bool)(response.get(2).asInt());

            return true;

        }

        return false;

    }



    /** Check if the current trajectory is terminated. Non blocking.

     * @return true if the trajectory is terminated, false otherwise

     */

    virtual bool checkMotionDone(bool *flag) { 

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(VOCAB_MOTION_DONES);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            *flag = (bool)(response.get(2).asInt());

			return true;

        }

        return false;

    }



    /** 

     * Set reference speed for a joint, this is the speed used during the

     * interpolation of the trajectory.

     * @param j joint number

     * @param sp speed value

     * @return true/false upon success/failure

     */

    virtual bool setRefSpeed(int j, double sp) { 

        return setDouble(VOCAB_REF_SPEED, j, sp);

    }



    /** 

     * Set reference speed on all joints. These values are used during the

     * interpolation of the trajectory.

     * @param spds pointer to the array of speed values.

     * @return true/false upon success/failure

     */

    virtual bool setRefSpeeds(const double *spds) { 

        return setDoubleArray(VOCAB_REF_SPEEDS, spds);

    }



    /** 

     * Set reference acceleration for a joint. This value is used during the

     * trajectory generation.

     * @param j joint number

     * @param acc acceleration value

     * @return true/false upon success/failure

     */

    virtual bool setRefAcceleration(int j, double acc) {

        return setDouble(VOCAB_REF_ACCELERATION, j, acc);

    }



    /** 

     * Set reference acceleration on all joints. This is the valure that is

     * used during the generation of the trajectory.

     * @param accs pointer to the array of acceleration values

     * @return true/false upon success/failure

     */

    virtual bool setRefAccelerations(const double *accs) { 

        return setDoubleArray(VOCAB_REF_ACCELERATIONS, accs);

    }



    /** 

     * Get reference speed for a joint. Returns the speed used to 

     * generate the trajectory profile.

     * @param j joint number

     * @param ref pointer to storage for the return value

     * @return true/false on success or failure

     */

    virtual bool getRefSpeed(int j, double *ref) {

        return getDouble(VOCAB_REF_SPEED, j, ref);

    }



    /** 

     * Get reference speed of all joints. These are the  values used during the

     * interpolation of the trajectory.

     * @param spds pointer to the array that will store the speed values.

     */

    virtual bool getRefSpeeds(double *spds) {

        return getDoubleArray(VOCAB_REF_SPEEDS, spds);

    }



    /** 

     * Get reference acceleration for a joint. Returns the acceleration used to 

     * generate the trajectory profile.

     * @param j joint number

     * @param acc pointer to storage for the return value

     * @return true/false on success/failure

     */

    virtual bool getRefAcceleration(int j, double *acc) {

        return getDouble(VOCAB_REF_ACCELERATION, j, acc);

    }



    /** 

     * Get reference acceleration of all joints. These are the values used during the

     * interpolation of the trajectory.

     * @param accs pointer to the array that will store the acceleration values.

     * @return true/false on success or failure 

     */

    virtual bool getRefAccelerations(double *accs) { 

        return getDoubleArray(VOCAB_REF_ACCELERATIONS, accs);

    }



    /** 

     * Stop motion, single joint

     * @param j joint number

     * @return true/false on success/failure

     */

    virtual bool stop(int j) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(VOCAB_STOP);

        cmd.addInt(j);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /** 

     * Stop motion, multiple joints 

     * @return true/false on success/failure

     */

    virtual bool stop() { 

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(VOCAB_STOPS);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /* IVelocityControl */



    /** 

     * Set new reference speed for a single axis.

     * @param j joint number

     * @param v specifies the new ref speed

     * @return true/false on success/failure

     */

    virtual bool velocityMove(int j, double v) {

        return setDouble(VOCAB_VELOCITY_MOVE, j, v);

    }



    /**

     * Set a new reference speed for all axes.

     * @param v is a vector of double representing the requested speed.

     * @return true/false on success/failure.

     */

    virtual bool velocityMove(const double *v) {

        CommandMessage& c = command_buffer.get();

		c.head.clear();

        c.head.addVocab(VOCAB_VELOCITY_MOVES);

        c.body.size(nj);

        ACE_OS::memcpy(&(c.body[0]), v, sizeof(double)*nj);

        command_buffer.write();

        return true;

    }



    /**

     * Set the controller to velocity mode.

     * @return true/false on success/failure.

     */

    virtual bool setVelocityMode() {

        return setCommand(VOCAB_VELOCITY_MODE);

    }



    /* IAmplifierControl */



    /** 

     * Enable the amplifier on a specific joint. Be careful, check that the output

     * of the controller is appropriate (usually zero), to avoid 

     * generating abrupt movements.

     * @return true/false on success/failure

     */

    virtual bool enableAmp(int j) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(VOCAB_AMP_ENABLE);

        cmd.addInt(j);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



	virtual bool setOffset(int j, double v)

	{

		Bottle cmd, response;

		cmd.addVocab(VOCAB_SET);

		cmd.addVocab(VOCAB_OFFSET);

		cmd.addInt(j);

		cmd.addDouble(v);



		bool ok = rpc_p.write(cmd, response);

		return CHECK_FAIL(ok, response);

	}



    /** 

     * Disable the amplifier on a specific joint. All computations within the board

     * will be carried out normally, but the output will be disabled.

     * @return true/false on success/failure

     */

    virtual bool disableAmp(int j) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(VOCAB_AMP_DISABLE);

        cmd.addInt(j);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }



    /**

     * Read the electric current going to all motors.

     * @param vals pointer to storage for the output values

     * @return hopefully true, false in bad luck.

     */

    virtual bool getCurrents(double *vals) {

        return getDoubleArray(VOCAB_AMP_CURRENTS, vals);

    }



    /** 

     * Read the electric current going to a given motor.

     * @param j motor number

     * @param val pointer to storage for the output value

     * @return probably true, might return false in bad time

     */

    virtual bool getCurrent(int j, double *val) {

        return getDouble(VOCAB_AMP_CURRENT, j, val);

    }



    /**

     * Set the maximum electric current going to a given motor. The behavior 

     * of the board/amplifier when this limit is reached depends on the

     * implementation.

     * @param j motor number

     * @param v the new value

     * @return probably true, might return false in bad time

     */

    virtual bool setMaxCurrent(int j, double v) {

        return setDouble(VOCAB_AMP_MAXCURRENT, j, v);

    }



    /**

     * Get the status of the amplifiers, coded in a 32 bits integer for

     * each amplifier (at the moment contains only the fault, it will be 

     * expanded in the future).

     * @param st pointer to storage

     * @return true in good luck, false otherwise.

     */

    virtual bool getAmpStatus(int *st) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(VOCAB_AMP_STATUS);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            *st = response.get(2).asInt();

            return true;

        }

        return false;        

    }



    /* IControlLimits */



    /**

     * Set the software limits for a particular axis, the behavior of the

     * control card when these limits are exceeded, depends on the implementation.

     * @param axis joint number

     * @param min the value of the lower limit

     * @param max the value of the upper limit

     * @return true or false on success or failure

     */

    virtual bool setLimits(int axis, double min, double max) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_SET);

        cmd.addVocab(VOCAB_LIMITS);

        cmd.addInt(axis);

        cmd.addDouble(min);

        cmd.addDouble(max);

        bool ok = rpc_p.write(cmd, response);

        return CHECK_FAIL(ok, response);

    }

    

    /**

     * Get the software limits for a particular axis.

     * @param axis joint number

     * @param min pointer to store the value of the lower limit

     * @param max pointer to store the value of the upper limit

     * @return true if everything goes fine, false if something bad happens

     */

    virtual bool getLimits(int axis, double *min, double *max) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(VOCAB_LIMITS);

        cmd.addInt(axis);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            *min = response.get(2).asDouble();

            *max = response.get(3).asDouble();

            return true;

        }

        return false;

    }







    /* IAxisInfo */

    

    virtual bool getAxisName(int j, yarp::os::ConstString& name) {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_GET);

        cmd.addVocab(VOCAB_INFO_NAME);

        cmd.addInt(j);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            name = response.get(2).asString();

            return true;

        }

        return false;

    }



    /* IControlCalibration */

    bool virtual calibrate()

    {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_CALIBRATE);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            return true;

        }

        return false;

    }



    bool virtual abortCalibration()

    {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_ABORTCALIB);

        bool ok=rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            return true;

        }

        return false;    

    }



    bool virtual abortPark()

    {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_ABORTPARK);

        bool ok=rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            return true;

        }

        return false;    

    }



    bool virtual park(bool wait=true)

    {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_PARK);

        cmd.addInt(wait);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            return true;

        }

        return false;

    }



    bool virtual calibrate2(int j, unsigned int ui, double v1, double v2, double v3)

    {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_CALIBRATE_JOINT);

        cmd.addInt(j);

        cmd.addInt(ui);

        cmd.addDouble(v1);

        cmd.addDouble(v2);

        cmd.addDouble(v3);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            return true;

        }

        return false;

    }



    bool virtual done(int j)

    {

        Bottle cmd, response;

        cmd.addVocab(VOCAB_CALIBRATE_DONE);

        cmd.addInt(j);

        bool ok = rpc_p.write(cmd, response);

        if (CHECK_FAIL(ok, response)) {

            return true;

        }

        return false;

    }



};
// implementation of CommandsHelper


yarp::dev::DriverCreator *createRemoteControlBoard() {
    return new DriverCreatorOf<RemoteControlBoard>("remote_controlboard", 
                                                   "controlboard",
                                                   "RemoteControlBoard");
}

