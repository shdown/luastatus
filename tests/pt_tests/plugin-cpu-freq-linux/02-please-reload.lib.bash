x_testcase_begin

x_add_or_set_cpu 0 1000000 500000 750000

x_my_callback() {
    case "$1" in
    0)
        true
        ;;
    1)
        pt_check x_add_or_set_cpu 0 20 10 30
        ;;
    2)
        pt_check x_add_or_set_cpu 0 9000000 3000000 2000000
        ;;
    3)
        pt_check x_add_or_set_cpu 0 20 10 15
        pt_check x_add_or_set_cpu 1 50 30 40
        ;;
    esac
}

x_testcase \
    1 \
    x_my_callback \
    'cb max=1000000,min=500000,cur=750000' \
    'cb max=20,min=10,cur=20' \
    'cb max=9000000,min=3000000,cur=3000000' \
    'cb max=20,min=10,cur=15 max=50,min=30,cur=40'

x_testcase_end
