#!/usr/bin/python3
import tempfile
import subprocess
import argparse
import fnmatch
import sys
import os

def find_files(basedir, file_ext):
    matches = []
    for filename in fnmatch.filter(os.listdir(basedir), '*.' + file_ext):
        matches.append(os.path.join(basedir, filename))
    return matches

def concat_cmd(fdlist, cmd, ext):
    s = ""
    for d in fdlist:
        if (not os.path.isdir(d) and "."+ext in d):
            s += cmd % (d)
        else:
            for f in find_files(d, ext):
                s += cmd % (f)
    return s

def read_hdl(library, dir_list):
    s = ""
    for d in dir_list:
        if (not os.path.isdir(d)):
            filename, file_extension = os.path.splitext(d)
            if (file_extension in [".vhd", ".vhdl"]):
                s += "read_vhdl -vhdl2008 -library %s %s\n" % (library, d)
            elif (file_extension in [".v"]):
                s += "read_verilog -library %s %s\n" % (library, d)
        else:
            for f in find_files(d, "vhd"):
                s += "read_vhdl -vhdl2008 -library %s %s\n" % (library, f)
            for f in find_files(d, "v"):
                s += "read_verilog -library %s %s\n" % (library, f)
    return s

def read_ip(dir_list):
    s = "set_property part $PART [current_project]\n\
set_property target_language VHDL [current_project]\n\
set_property default_lib work [current_project]\n\n\
"
    s += concat_cmd(dir_list, "read_ip %s\n", "xci",)

    return s

def read_xdc(dir_list):
    s = concat_cmd(dir_list, "read_xdc %s\n", "xdc")
    return s

def post_synth(dir_list):
    if (dir_list == None):
        return ""
    s = "# Post synth\n"
    s += concat_cmd(dir_list, "read_xdc %s\n", "xdc")
    s += "\n"
    return s

def build_ip(dir_list):
    s = concat_cmd(dir_list, "generate_target all [get_files  %s]\n", "xci")
    s += "synth_ip [get_ips]\n"
    return s

def synth(dcp):
    if (dcp == None):
        dcp = "bin/post_synth.dcp"

    s = "\n# Run Synthesis\n\
synth_design -top $TOP_MODULE -part $PART\n\
write_checkpoint -force %s\n\
report_timing_summary -file ./log/post_synth_timing_summary.rpt\n\
report_utilization -file ./log/post_synth_util.rpt\n\n" % (dcp)
    return s

def opt(dcp):
    if (dcp == None):
        dcp = "bin/post_place.dcp"

    s = "# Optimization\n\
opt_design\n\n\
# Place\n\
#place_ports\n\
place_design\n\
report_clock_utilization -file ./log/clock_util.rpt\n\
#Get timing violations and run optimizations if needed\n\
if {[get_property SLACK [get_timing_paths -max_paths 1 -nworst 1 -setup]] < 0} {\n\
 puts \"Found setup timing violations => running physical optimization\"\n\
 phys_opt_design\n\
}\n\
write_checkpoint -force %s\n\
report_utilization -file log/post_place_util.rpt\n\
report_timing_summary -file log/post_place_timing_summary.rpt\n\n" % (dcp)
    return s

def route(dcp):
    if (dcp == None):
        dcp = "bin/post_route.dcp"

    s = "# Route\n\
route_design\n\
getTimingInfo\n\
write_checkpoint -force %s\n\
report_route_status -file log/post_route_status.rpt\n\
report_timing_summary -file log/post_route_timing_summary.rpt\n\
report_power -file log/post_route_power.rpt\n\
report_drc -file log/post_imp_drc.rpt\n\n\
" % (dcp)
    return s

def bitstream(bit):
    if (bit == None):
        bit = "bin/$TOP_MODULE.bit"
    s = "# Bitstream and debug probes\n\
write_bitstream -force %s\n\
write_debug_probes -force ${TOP_MODULE}_debug_probes.ltx\n\n" % (bit)
    return s

def preamble(fpga_part, top_module):
    s = "set_param general.maxBackupLogs 0\n\
set TOP_MODULE %s\n\
set PART %s\n\n\
source %s/top.tcl\n\n\
" % (top_module, fpga_part, os.path.dirname(os.path.abspath(__file__)))

    return s

def post_read():
    s = "set_property part $PART [current_project]\n\
set_property target_language VHDL [current_project]\n\
set_property default_lib work [current_project]\n\n\
"
    return s

def checkpoint(dcp, default):
    fout = "open_checkpoint %s\n" % (default)
    if (dcp != None):
        fout = "open_checkpoint %s\n" % (args.dcp)
    return fout

def make_vivado_project():
    s = "file mkdir ./vivado/\n\
# Vivado project directory\n\
set project_dir ./vivado/\n\
\n\
#Close currently open project and create a new one. (OVERWRITES PROJECT!!)\n\
close_project -quiet\n\
create_project -force -part $PART $TOP_MODULE ./vivado/\n\
set_property top $TOP_MODULE [current_fileset]\n\
"
    return s

def launch_vivado(tcl_file):
    stats = []
    cmd = ["vivado", "-mode", "batch", "-nojournal", "-log", "./log/vivado.log", "-notrace", "-source", tcl_file]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for lout in proc.stdout:
        lout = lout.decode(encoding='utf-8')
        if ("~R~" in lout):
            stats.append(lout[3:])
        print(lout, end="")

    for lerr in proc.stderr:
        lerr = lerr.decode(encoding='utf-8')
        print(lerr, end="")

    for l in stats:
        print(l, end="")

def main(args):
    fout = ""
    fout += preamble(args.fpga, args.top)

    if (args.type == "project"):
        fout += make_vivado_project()

    if (args.type in ["all", "synth", "project"]):
        if (args.hdl != None):
            fout += read_hdl("xil_library", args.hdl)

        if (args.xdc != None):
            fout += read_xdc(args.xdc)
            fout += "\n"

        if (args.ip != None):
            fout += read_ip(args.ip)

    if (args.type == "all"):
        fout += synth(None)
        fout += post_synth(args.postsynth)
        fout += opt(None)
        fout += route(None)
        fout += bitstream(args.out)
    elif (args.type == "synth"):
        fout += synth(args.out)
        fout += post_synth(args.postsynth)
    elif (args.type == "continue"):
        fout += "create_project -in_memory\n"
        fout += post_read()
        fout += checkpoint(args.dcp, "bin/post_synth.dcp")
        fout += post_synth(args.postsynth)
        fout += opt(args.out)
        fout += route(None)
        fout += bitstream(args.out)
    elif (args.type == "opt"):
        fout += "create_project -in_memory\n"
        fout += post_read()
        fout += checkpoint(args.dcp, "bin/post_synth.dcp")
        fout += post_synth(args.postsynth)
        fout += opt(args.out)
    elif (args.type == "route"):
        fout += "create_project -in_memory\n"
        fout += post_read()
        fout += checkpoint(args.dcp, "bin/post_place.dcp")
        fout += route(args.out)
    elif (args.type == "bitstream"):
        fout += "create_project -in_memory\n"
        fout += post_read()
        fout += checkpoint(args.dcp, "bin/post_route.dcp")
        fout += post_synth(args.postsynth)
        fout += bitstream(args.out)
    elif (args.type == "ip"):
        fout += "create_project -in_memory\n"
        fout += read_ip(args.ip)
        fout += build_ip(args.ip)

    if (args.dry):
        print(fout)
        return

    #f = open("./build.tcl", "w")
    #f.write(fout)
    #f.close()

    #launch_vivado("./build.tcl")

    fp = tempfile.NamedTemporaryFile(mode="w", prefix="vivado_build_", suffix=".tcl")
    fp.write(fout)
    fp.seek(0)
    launch_vivado(fp.name)
    fp.close()


if (__name__ == "__main__"):
    parser = argparse.ArgumentParser(description="Generate build.tcl script for Vivado")
    parser.add_argument("type", help="all, synth, opt, route, bitstream ip")
    parser.add_argument('-d','--dcp', help='Design checkpoint to use (.dcp)')
    parser.add_argument('-l','--hdl', nargs='+', help='List of VHDL files or dirs (.vhd)')
    parser.add_argument('-i','--ip', nargs='+', help='List of IP files or dir (.xci)')
    parser.add_argument('-x','--xdc', nargs='+', help='List of xdc files or dir (.xdc)')
    parser.add_argument('--postsynth', nargs='+', help='List of xdc files or dir (.xdc) post synth')
    parser.add_argument('-f','--fpga', help='FPGA Part Number', required=True)
    parser.add_argument('-t','--top', help='Top Module')
    parser.add_argument('-o','--out', help='Output File (.bit or .dcp)')
    parser.add_argument('--dry', action='store_true', help='Dry Run')

    args = parser.parse_args()
    main(args)
