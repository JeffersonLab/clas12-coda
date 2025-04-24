proc getTimingInfo { {report {}} } {
   if {$report == {}} { 
      set report [split [report_timing_summary -no_detailed_paths -no_check_timing -no_header -return_string] \n]
   } else {
      set report [split $report \n]
   }

   foreach {wns tns tnsFailingEp tnsTotalEp whs ths thsFailingEp thsTotalEp wpws tpws tpwsFailingEp tpwsTotalEp} [list {N/A} {N/A} {N/A} {N/A} {N/A} {N/A} {N/A} {N/A} {N/A} {N/A} {N/A} {N/A}] { 
      break 
   }
   if {[set i [lsearch -regexp $report {Design Timing Summary}]] != -1} {
      foreach {wns tns tnsFailingEp tnsTotalEp whs ths thsFailingEp thsTotalEp wpws tpws tpwsFailingEp tpwsTotalEp} [regexp -inline -all -- {\S+} [lindex $report [expr $i + 6]]] { 
         break 
      }
   }
   puts stderr "~R~ Setup:       |  WNS: $wns  |  TNS: $tns  | Failing Endpoints: $tnsFailingEp | Total Endpoints: $tnsTotalEp |"
   puts stderr "~R~ Hold:        |  WHS: $whs  |  THS: $ths  | Failing Endpoints: $thsFailingEp | Total Endpoints: $thsTotalEp |"
   puts stderr "~R~ Pulse Width: | WPWS: $wpws  | TPWS: $tpws  | Failing Endpoints: $tpwsFailingEp | Total Endpoints: $tpwsTotalEp |\n\n"
}
