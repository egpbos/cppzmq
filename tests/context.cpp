#include <catch.hpp>
#include <zmq.hpp>

#include <cerrno>
#include <unistd.h> // ualarm
#include <csignal> // kill
//#include <wait.h> // waitpid
#include <sys/ptrace.h>

TEST_CASE("context construct default and destroy", "[context]")
{
    zmq::context_t context;
}

TEST_CASE("context create, close and destroy", "[context]")
{
    zmq::context_t context;
    context.close();
    CHECK(NULL == (void *) context);
}

TEST_CASE("context close, dealing with EINTR", "[context]")
{
//    for (int i = 0; i < 100; ++i) {
        int status;
        pid_t child_pid = fork();
        if (child_pid == 0) {
            // in child process

            status = ptrace(PT_ATTACHEXC, getppid(), 0, 0);
            assert(status);


            zmq::context_t context;
            zmq::socket_t socket(context, ZMQ_PAIR);
            socket.connect("ipc:///tmp/test_context_close_eintr.ipc");

            zmq::message_t reply (5);
            memcpy (reply.data (), "World", 5);
            socket.send (reply);

            socket.close();
            context.close();

            for (;;) {
                /* Enter next system call */
                ptrace(PTRACE_SYSCALL, pid, 0, 0);
                waitpid(pid, 0, 0);

                struct user_regs_struct regs;
                ptrace(PTRACE_GETREGS, pid, 0, &regs);

                /* Is this system call permitted? */
                int blocked = 0;
                if (is_syscall_blocked(regs.orig_rax)) {
                    blocked = 1;
                    regs.orig_rax = -1; // set to invalid syscall
                    ptrace(PTRACE_SETREGS, pid, 0, &regs);
                }

                /* Run system call and stop on exit */
                ptrace(PTRACE_SYSCALL, pid, 0, 0);
                waitpid(pid, 0, 0);

                if (blocked) {
                    /* errno = EPERM */
                    regs.rax = -EPERM; // Operation not permitted
                    ptrace(PTRACE_SETREGS, pid, 0, &regs);
                }
            }

            _Exit(0);
        } else {
            // in parent process

            pid_t wait_child = wait(&status);
            assert(child_pid == wait_child);

            zmq::context_t context;
            zmq::socket_t socket(context, ZMQ_PAIR);
            socket.bind("ipc:///tmp/test_context_close_eintr.ipc");

            zmq::message_t request;
            socket.recv(&request);

//            waitpid(child_pid, nullptr, WNOHANG); // don't handle 0 (== child status unchanged)
            socket.close();
//            ualarm(150000, 0);
            context.close();
//            CHECK(context.close() != -1);
//            CHECK(zmq::error_t().num() != 4);
        }
//    }
}


//TEST_CASE("context close, dealing with EINTR", "[context]")
//{
//  zmq::context_t context;
//  zmq::socket_t sender(context, ZMQ_PAIR);
//  context.close();
////    errno = EINTR;
//}
