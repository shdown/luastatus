nonexisting_dir=/tzmicdb8kf38onA67olhZiyd79xml3

# Without trailing '/'
is_program_running_testcase \
    "kind='dir_nonempty', path='${nonexisting_dir}'" \
    x_do_nothing \
    'cb false'

# With trailing '/'
is_program_running_testcase \
    "kind='dir_nonempty', path='${nonexisting_dir}/'" \
    x_do_nothing \
    'cb false'
