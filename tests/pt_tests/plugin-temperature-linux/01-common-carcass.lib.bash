### --- Generated with: ---
###########################
# (
#     set -e
#
#     echo_D() {
#         local path=$1
#         if [ -e "$path" ]; then
#             echo "D $path _ _ _"
#         fi
#     }
#
#     echo_F() {
#         local path=$1
#         local kind=$2
#         local name=$3
#         if [ -e "$path" ]; then
#             echo "F $path $(cat < "$path") $kind $name"
#         fi
#     }
#
#     cd /sys/class
#
#     for d in thermal/thermal_zone*; do
#         echo_D "$d"
#         echo_F "$d"/temp thermal "${d##*/}"
#     done
#
#     for d in hwmon/*; do
#         echo_D "$d"
#         echo_F "$d"/name _ _
#         IFS= read -r name < "$d"/name || continue
#         for f in "$d"/temp*_input; do
#             echo_F "$f" hwmon "$name"
#         done
#     done
# )

common_carcass="\
D thermal/thermal_zone0 _ _ _
F thermal/thermal_zone0/temp 43000 thermal thermal_zone0
D thermal/thermal_zone1 _ _ _
F thermal/thermal_zone1/temp 20000 thermal thermal_zone1
D thermal/thermal_zone2 _ _ _
F thermal/thermal_zone2/temp 37050 thermal thermal_zone2
D thermal/thermal_zone3 _ _ _
F thermal/thermal_zone3/temp 42050 thermal thermal_zone3
D thermal/thermal_zone4 _ _ _
F thermal/thermal_zone4/temp 40050 thermal thermal_zone4
D thermal/thermal_zone5 _ _ _
F thermal/thermal_zone5/temp 50 thermal thermal_zone5
D thermal/thermal_zone6 _ _ _
F thermal/thermal_zone6/temp 50 thermal thermal_zone6
D thermal/thermal_zone7 _ _ _
F thermal/thermal_zone7/temp 32650 thermal thermal_zone7
D thermal/thermal_zone8 _ _ _
F thermal/thermal_zone8/temp 43050 thermal thermal_zone8
D thermal/thermal_zone9 _ _ _
F thermal/thermal_zone9/temp 43000 thermal thermal_zone9
D hwmon/hwmon0 _ _ _
F hwmon/hwmon0/name acpitz _ _
F hwmon/hwmon0/temp1_input 43000 hwmon acpitz
D hwmon/hwmon1 _ _ _
F hwmon/hwmon1/name BAT0 _ _
D hwmon/hwmon2 _ _ _
F hwmon/hwmon2/name nvme _ _
F hwmon/hwmon2/temp1_input 36850 hwmon nvme
D hwmon/hwmon3 _ _ _
F hwmon/hwmon3/name ADP1 _ _
D hwmon/hwmon4 _ _ _
F hwmon/hwmon4/name coretemp _ _
F hwmon/hwmon4/temp1_input 42000 hwmon coretemp
F hwmon/hwmon4/temp2_input 39000 hwmon coretemp
F hwmon/hwmon4/temp3_input 40000 hwmon coretemp
F hwmon/hwmon4/temp4_input 39000 hwmon coretemp
F hwmon/hwmon4/temp5_input 39000 hwmon coretemp
"
