/*
* Copyright (C) 2020 Romain Feron
* This file is part of RADSex.

* RADSex is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* RADSex is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with RADSex.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "arg_parser.h"
#include "depth.h"
//#include "distrib.h"
//#include "freq.h"
//#include "map.h"
#include "parameters.h"
//#include "process.h"
//#include "signif.h"
//#include "subset.h"

// Argument parsing main function
inline Parameters parse_args(int& argc, char** argv) {

    CLI::App parser {""};  // Parser instance from CLI App parser
    Parameters parameters;
    std::vector<std::string> groups;

    std::shared_ptr<CustomFormatter> formatter(new CustomFormatter);

    // Main parser options
    parser.formatter(formatter);  // Set custom help format defined above
    parser.require_subcommand();  // Check that there is a subcommand
    parser.failure_message(failure_message);  // Formatting for error message

    // Depths subcommand parser
    CLI::App* depth = parser.add_subcommand("depth", "Compute number of retained reads for each individual");
    depth->add_option("-t,--markers-table", parameters.markers_table_path, "Path to a marker depths table generated by \"process\"")->required()->check(CLI::ExistingFile);
    depth->add_option("-p,--popmap", parameters.popmap_file_path, "Path to a tabulated map specifying groups for all individuals (population map)")->required()->check(CLI::ExistingFile);
    depth->add_option("-o,--output-file", parameters.output_file_path, "Path to the output file (table of depth for each individual)")->required();

    // Distrib subcommand parser
    CLI::App* distrib = parser.add_subcommand("distrib", "Compute the distribution of markers between group1 and group2");
    distrib->add_option("-t,--markers-table", parameters.markers_table_path, "Path to a marker depths table generated by \"process\"")->required()->check(CLI::ExistingFile);
    distrib->add_option("-p,--popmap", parameters.popmap_file_path, "Path to a tabulated file specifying groups for all individuals (population map)")->required()->check(CLI::ExistingFile);
    distrib->add_option("-o,--output-file", parameters.output_file_path, "Path to the output file (distribution of markers between groups)")->required();
    distrib->add_option("-d,--min-depth", parameters.min_depth, "Minimum depth to consider a marker present in an individual", true)->check(CLI::Range(1, 9999));
    distrib->add_option("-G,--groups", groups, "Names of the groups to compare if there are more than two groups in the popmap (--groups group1,group2)")->delimiter(',');
    distrib->add_option("-S,--signif-threshold", parameters.signif_threshold, "P-value threshold to consider a marker significantly associated with a phenotypic group", true)->check(CLI::Range(0.0, 1.0));
    distrib->add_flag("-C,--disable-correction", parameters.disable_correction, "If set, Bonferroni correction will NOT be used when assessing significance of association with phenotypic group");
    distrib->add_flag("-x,--output-matrix", parameters.output_matrix, "If set, the distribution will be output as a matrix instead of a table");

    // Freq subcommand parser
    CLI::App* freq = parser.add_subcommand("freq", "Compute marker frequencies in all individuals");
    freq->add_option("-t,--markers-table", parameters.markers_table_path, "Path to a marker depths table generated by \"process\"")->required()->check(CLI::ExistingFile);
    freq->add_option("-o,--output-file", parameters.output_file_path, "Path to the output file (distribution of marker frequencies in all individuals)")->required();
    freq->add_option("-d,--min-depth", parameters.min_depth, "Minimum depth to consider a marker present in an individual", true)->check(CLI::Range(1, 9999));

    // Map subcommand parser
    CLI::App* map = parser.add_subcommand("map", "Align markers to a genome and compute metrics for each aligned marker");
    map->add_option("-s,--markers-file", parameters.markers_table_path, "Path to a set of markers to align, either a depth table from \"process\", \"signif\", or \"subset\" or a fasta file from \"subset\" or \"signif\"")->required()->check(CLI::ExistingFile);
    map->add_option("-o,--output-file", parameters.output_file_path, "Path to the output file (mapping position, group bias, and probability of association with group for all aligned markers)")->required();
    map->add_option("-p,--popmap", parameters.popmap_file_path, "Path to a tabulated file specifying groups for all individuals (population map)")->required()->check(CLI::ExistingFile);
    map->add_option("-g,--genome-file", parameters.genome_file_path, "Path to the genome file in fasta format")->required()->check(CLI::ExistingFile);
    map->add_option("-d,--min-depth", parameters.min_depth, "Minimum depth to consider a marker present in an individual", true)->check(CLI::Range(1, 9999));
    map->add_option("-G,--groups", groups, "Names of the groups to compare if there are more than two groups in the popmap (--groups group1,group2)")->delimiter(',');
    map->add_option("-q,--min-quality", parameters.map_min_quality, "Minimum mapping quality to retain a read", true)->check(CLI::Range(0, 60));
    map->add_option("-Q,--min-frequency", parameters.map_min_frequency, "Minimum frequency of individuals to retain a marker", true)->check(CLI::Range(0.0, 1.0));
    map->add_option("-S,--signif-threshold", parameters.signif_threshold, "P-value threshold to consider a marker significantly associated with a phenotypic group", true)->check(CLI::Range(0.0, 1.0));
    map->add_flag("-C,--disable-correction", parameters.disable_correction, "If set, Bonferroni correction will NOT be used when assessing significance of association with phenotypic group");

    // Process subcommand parser
    CLI::App* process = parser.add_subcommand("process", "Compute a table of marker depths from a set of demultiplexed reads files");
    process->add_option("-i,--input-dir", parameters.input_dir_path, "Path to a directory contains demultiplexed sequence files")->required()->check(CLI::ExistingDirectory);
    process->add_option("-o,--output-file", parameters.output_file_path, "Path to the output file (table of marker depths in each individual)")->required();
    process->add_option("-T,--threads", parameters.n_threads, "Number of threads to use", true)->check(CLI::Range(1, 9999));
    process->add_option("-d,--min-depth", parameters.min_depth, "Minimum depth in at least one individual to retain a marker", true)->check(CLI::Range(1, 9999));

    // Signif subcommand parser
    CLI::App* signif = parser.add_subcommand("signif", "Extract markers significantly associated with phenotypic group from a marker depths table");
    signif->add_option("-t,--markers-table", parameters.markers_table_path, "Path to a marker depths table generated by \"process\"")->required()->check(CLI::ExistingFile);
    signif->add_option("-p,--popmap", parameters.popmap_file_path, "Path to a tabulated file specifying groups for all individuals (population map)")->required()->check(CLI::ExistingFile);
    signif->add_option("-o,--output-file", parameters.output_file_path, "Path to the output file (marker depths table or fasta file for markers significantly associated with phenotypic group)")->required();
    signif->add_option("-d,--min-depth", parameters.min_depth, "Minimum depth to consider a marker present in an individual", true)->check(CLI::Range(1, 9999));
    signif->add_option("-G,--groups", groups, "Names of the groups to compare if there are more than two groups in the popmap (--groups group1,group2)")->delimiter(',');
    signif->add_option("-S,--signif-threshold", parameters.signif_threshold, "P-value threshold to consider a marker significantly associated with a phenotypic group", true)->check(CLI::Range(0.0, 1.0));
    signif->add_flag("-C,--disable-correction", parameters.disable_correction, "If set, Bonferroni correction will NOT be used when assessing significance of association with phenotypic group");
    signif->add_flag("-a,--output-fasta", parameters.output_fasta, "If set, markers will be output in fasta format instead of table format");

    // Subset subcommand parser
    CLI::App* subset = parser.add_subcommand("subset", "Extract a subset of a marker depths table");
    subset->add_option("-t,--markers-table", parameters.markers_table_path, "Path to a marker depths table generated by \"process\"")->required()->check(CLI::ExistingFile);
    subset->add_option("-p,--popmap", parameters.popmap_file_path, "Path to a tabulated file specifying groups for all individuals (population map)")->required()->check(CLI::ExistingFile);
    subset->add_option("-o,--output-file", parameters.output_file_path, "Path to the output file (marker depths table or fasta file for extracted markers)")->required();
    subset->add_option("-d,--min-depth", parameters.min_depth, "Minimum depth to consider a marker present in an individual", true)->check(CLI::Range(1, 9999));
    subset->add_option("-G,--groups", groups, "Names of the groups to compare if there are more than two groups in the popmap (--groups group1,group2)")->delimiter(',');
    subset->add_option("-S,--signif-threshold", parameters.signif_threshold, "P-value threshold to consider a marker significantly associated with a phenotypic group", true)->check(CLI::Range(0.0, 1.0));
    subset->add_flag("-C,--disable-correction", parameters.disable_correction, "If set, Bonferroni correction will NOT be used when assessing significance of association with phenotypic group");
    subset->add_flag("-a,--output-fasta", parameters.output_fasta, "If set, markers will be output in fasta format instead of table format");
    subset->add_option("-m,--min-group1", parameters.subset_min_group1, "Minimum number of individuals from the first group to retain a marker in the subset", true)->check(CLI::Range(0, 9999));
    subset->add_option("-n,--min-group2", parameters.subset_min_group2, "Minimum number of individuals from the second group to retain a marker in the subset", true)->check(CLI::Range(0, 9999));
    subset->add_option("-M,--max-group1", parameters.subset_max_group1, "Maximum number of individuals from the first group to retain a marker in the subset", true)->check(CLI::Range(0, 9999));
    subset->add_option("-N,--max-group2", parameters.subset_max_group2, "Maximum number of individuals from the second group to retain a marker in the subset", true)->check(CLI::Range(0, 9999));
    subset->add_option("-i,--min-individuals", parameters.subset_min_individuals, "Minimum number of individuals to retain a marker in the subset", true)->check(CLI::Range(0, 9999));
    subset->add_option("-I,--max-individuals", parameters.subset_max_individuals, "Maximum number of individuals to retain a marker in the subset", true)->check(CLI::Range(0, 9999));

    // The parser throws an exception upon failure and implements an exit() method which output an error message and returns the exit code.
    try {

        parser.parse(argc, argv);

    } catch (const CLI::ParseError &e) {

        if (parser.get_subcommands().size() > 0) {
            formatter->set_column_widths(parser);
        } else {
            formatter->column_widths[0] = 10;
            formatter->column_widths[1] = 0;
            formatter->column_widths[2] = 50;
        }

        exit(parser.exit(e));

    }

    // Get subcommand name
    CLI::App* subcommand = parser.get_subcommands()[0];

    // Set max_<N> for the 'subset' command if max was not specified by the user
    if (subcommand->get_name() == "subset") {
        if (subcommand->count("--max-group1")) parameters.set_max_group1 = false;
        if (subcommand->count("--max-group2")) parameters.set_max_group2 = false;
        if (subcommand->count("--max-individuals")) parameters.set_max_individuals = false;
    }

    // Set groups to compare if specified by user
    if (groups.size() == 2) {
        parameters.group1 = groups[0];
        parameters.group2 = groups[1];
    }

    // Store subcommand name
    parameters.command = subcommand->get_name();

    return parameters;
}



int main(int argc, char* argv[]) {

    /* RADSex is the main class implementing the program.
     * It makes use of the CLI11 library to parse arguments and call the appropriate function.
     */

    // Map of function to run for each command


    // Get parameter values from parsing command-line arguments
    Parameters parameters = parse_args(argc, argv);

    Depth analysis(parameters, false, true, false);
    analysis.run();

    return 0;
}



