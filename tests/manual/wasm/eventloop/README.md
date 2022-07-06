Event loop exec() and main() on Qt for WebAssembly
==================================================

These examples demonstrate how QEventLoop::exec() works on
Qt for WebAssembly, and also shows how to implement main()
without calling QApplication::exec().

Contents
========

    main_exec       Standard Qt main(), where QApplication::exec() does not return
    main_noexec     Qt main() without QApplication::exec()
    dialog_exec     Shows how QDialog::exec() also does not return
    thread_exec     Shows how to use QThread::exec()
    eventloop_auto  Event loop autotest (manually run)
