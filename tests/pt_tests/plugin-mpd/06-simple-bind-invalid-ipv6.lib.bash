pt_testcase_begin

# This is a malformed IPv4-mapped IPv6 address.
# It should fail regardless of whether or not the system we are running on supports IPv6.
x_mpd_simple_template ipv6/'::ffff:256.255.255.255' 1

pt_testcase_end
