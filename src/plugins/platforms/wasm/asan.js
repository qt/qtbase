// In particular log_path is important, otherwise there will not be any output
Module.ASAN_OPTIONS= 'log_path=stdout:abort_on_error=0:halt_on_error=0:print_stacktrace=1:keep_going=1:verbosity=1:debug=1:check_initialization_order=1:detect_stack_use_after_return=1:detect_stack_use_after_scope=1:print_full_thread_history=1:strict_string_checks=1:detect_invalid_pointer_pairs=5:symbolize=1:fast_unwind_on_malloc=0';
Module.UBSAN_OPTIONS='log_path=stdout:abort_on_error=0:halt_on_error=0:print_stacktrace=1:keep_going=1';
