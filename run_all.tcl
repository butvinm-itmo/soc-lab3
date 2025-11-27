############################################################
## HLS Automation Script for Matrix Operations Variants
## Compares: Baseline, Unrolled, and Pipelined implementations
############################################################

# Project settings
set project_name "mat_comparison"
set top_function "mat_compute"
set part_name "xc7a100t-csg324-1"
set clock_period 10

# List of variants to synthesize
set variants {
    {baseline mat_baseline.c "No optimization"}
    {unroll   mat_unroll.c   "Loop unrolling"}
    {pipeline mat_pipeline.c "Loop pipelining"}
}

# Process each variant
foreach variant $variants {
    set sol_name [lindex $variant 0]
    set src_file [lindex $variant 1]
    set description [lindex $variant 2]

    puts "============================================"
    puts "Processing: solution_$sol_name ($description)"
    puts "Source file: $src_file"
    puts "============================================"

    # Create fresh project for each variant to avoid file conflicts
    open_project ${project_name}_${sol_name} -reset

    # Set top function
    set_top $top_function

    # Create solution
    open_solution "solution" -reset

    # Add source files (only this variant's implementation)
    add_files $src_file
    add_files -tb mat_test.c -cflags "-Wno-unknown-pragmas"
    add_files -tb out.gold.dat

    # Set target device
    set_part $part_name -tool vivado

    # Set clock constraint (100 MHz)
    create_clock -period $clock_period -name default

    # Configure export settings
    config_export -format ip_catalog -rtl verilog

    # Run C simulation
    puts "Running C simulation for $sol_name..."
    csim_design

    # Run C synthesis
    puts "Running C synthesis for $sol_name..."
    csynth_design

    # Run co-simulation with waveform generation
    puts "Running co-simulation for $sol_name..."
    cosim_design -trace_level all

    # Export design
    puts "Exporting design for $sol_name..."
    export_design -rtl verilog -format ip_catalog
}

puts ""
puts "============================================"
puts "All variants processed successfully!"
puts "============================================"
puts ""
puts "Synthesis reports are available at:"
puts "  - ${project_name}_baseline/solution/syn/report/${top_function}_csynth.rpt"
puts "  - ${project_name}_unroll/solution/syn/report/${top_function}_csynth.rpt"
puts "  - ${project_name}_pipeline/solution/syn/report/${top_function}_csynth.rpt"

exit
