#include "signif.h"

void signif(Parameters& parameters) {

    /* The sex_distribution function parses through a file generated by process_reads and checks for each sequence
     * the number of males and females in which the sequence was found. The output is a table with five columns:
     * Number of males | Number of females | Number of sequences | P-value | Significant
     *     <int>       |       <int>       |       <int>         | <float> |   <bool>
     */

    std::chrono::steady_clock::time_point t_begin = std::chrono::steady_clock::now();
    Popmap popmap = load_popmap(parameters);

    log("RADSex signif started");
    log("Comparing groups \"" + parameters.group1 + "\" and \"" + parameters.group2 + "\"");

    Header header;

    std::vector<Marker> candidate_markers;
    uint n_markers;

    bool parsing_ended = false;
    MarkersQueue markers_queue;
    std::mutex queue_mutex;

    std::thread parsing_thread(table_parser, std::ref(parameters), std::ref(popmap), std::ref(markers_queue), std::ref(queue_mutex), std::ref(header), std::ref(parsing_ended), false, false);
    std::thread processing_thread(processor, std::ref(markers_queue), std::ref(popmap), std::ref(parameters), std::ref(queue_mutex), std::ref(candidate_markers), std::ref(n_markers), std::ref(parsing_ended), BATCH_SIZE);

    parsing_thread.join();
    processing_thread.join();

    std::ofstream output_file = open_output(parameters.output_file_path);

    if (not parameters.disable_correction) parameters.signif_threshold /= n_markers; // Bonferroni correction: divide threshold by number of tests

    // Second pass: filter markers with threshold corrected p < 0.05
    for (auto marker: candidate_markers) {

        if (static_cast<float>(marker.p) < parameters.signif_threshold) {

            parameters.output_fasta ? marker.output_fasta(output_file, parameters.min_depth) : marker.output_table(output_file);

        }

    }

    output_file.close();

    log("RADSex signif ended (total runtime: " + get_runtime(t_begin) + ")");
}


void processor(MarkersQueue& markers_queue, Popmap& popmap, Parameters& parameters, std::mutex& queue_mutex, std::vector<Marker>& candidate_markers, uint& n_markers, bool& parsing_ended, ulong batch_size) {

    // Give 100ms headstart to table parser thread (to get number of individuals from header)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<Marker> batch;
    bool keep_going = true;

    double chi_squared = 0;

    uint marker_processed_tick = static_cast<uint>(markers_queue.n_markers / 100);
    uint64_t n_processed_markers = 0;

    while (keep_going) {

        // Get a batch of markers from the queue
        batch = get_batch(markers_queue, queue_mutex, batch_size);

        if (batch.size() > 0) {  // Batch not empty

            for (auto& marker: batch) {

                if (marker.n_individuals > 0) {

                    ++n_markers;  // Increment total number of markers for Bonferroni correction
                    chi_squared = get_chi_squared(marker.groups[parameters.group1], marker.groups[parameters.group2], popmap.counts[parameters.group1], popmap.counts[parameters.group2]);
                    marker.p = static_cast<float>(get_chi_squared_p(chi_squared));
                    // First pass: filter markers with non-corrected p < 0.05
                    if (static_cast<float>(marker.p) < parameters.signif_threshold) candidate_markers.push_back(marker);

                }

                log_progress_bar(n_processed_markers, marker_processed_tick);

            }

        } else {

            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Batch empty: wait 10ms before asking for another batch

        }

        if (parsing_ended and markers_queue.markers.size() == 0) keep_going = false;
    }
}
