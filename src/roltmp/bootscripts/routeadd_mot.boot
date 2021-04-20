#  routeadd_mot.boot

# adds router : all traffic from anywhere will goto 129.57.68.100 router 
#routeNetAdd "0.0.0.0","129.57.68.100"
#routeNetAdd "0.0.0.0","129.57.167.99"


#routeNetAdd "129.57.167.0","129.57.68.100"

# allow access from 86 subnet
routeNetAdd "129.57.86.0","129.57.167.99"

