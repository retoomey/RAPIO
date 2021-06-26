# Toomey, run a netcdf test based on an archive case I have.
# Can modify the -i to any code_index.xml you have handy
rm -rf NETCDFTEST
./rTestNetcdf -n="disable" -i /home/dyolf/DATA3/satellite/code_index.xml -I="*" -o "netcdf=NETCDFTEST"
