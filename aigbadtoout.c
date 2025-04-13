#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "aiger.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.aig/aag> <output.aig/aag>\n", argv[0]);
        return 1;
    }

    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    
    // Check input file existence
    if (access(input_filename, R_OK) != 0) {
        perror("Input file access error");
        return 1;
    }
    
    // Determine if input and output are ASCII or binary format
    int is_ascii_input = (strlen(input_filename) > 4 && 
                          strcmp(input_filename + strlen(input_filename) - 4, ".aag") == 0);
    int is_ascii_output = (strlen(output_filename) > 4 && 
                           strcmp(output_filename + strlen(output_filename) - 4, ".aag") == 0);
    
    printf("Input format: %s\n", is_ascii_input ? "ASCII (.aag)" : "Binary (.aig)");
    printf("Output format: %s\n", is_ascii_output ? "ASCII (.aag)" : "Binary (.aig)");
    
    // Initialize AIGER model
    aiger *model = aiger_init();
    if (!model) {
        fprintf(stderr, "Failed to initialize AIGER model\n");
        return 1;
    }
    
    const char *error_msg = NULL;
    
    // Read the model using appropriate function for the format
    if (is_ascii_input) {
        FILE *file = fopen(input_filename, "r");
        if (!file) {
            perror("Failed to open input file");
            aiger_reset(model);
            return 1;
        }
        printf("Reading ASCII AIGER model...\n");
        error_msg = aiger_read_from_file(model, file);
        fclose(file);
    } else {
        printf("Reading binary AIGER model...\n");
        error_msg = aiger_open_and_read_from_file(model, input_filename);
    }
    
    if (error_msg) {
        fprintf(stderr, "Failed to read AIGER model: %s\n", error_msg);
        aiger_reset(model);
        return 1;
    }
    
    printf("Successfully read AIGER model\n");
    printf("Model stats: M=%u, I=%u, L=%u, O=%u, A=%u, B=%u, C=%u\n",
           model->maxvar, model->num_inputs, model->num_latches,
           model->num_outputs, model->num_ands, model->num_bad, model->num_constraints);
    
    // Check if the model has bad states
    if (model->num_bad == 0) {
        fprintf(stderr, "Model has no bad states to convert\n");
        aiger_reset(model);
        return 1;
    }
    
    // Convert bad states to outputs
    printf("Converting %u bad states to outputs...\n", model->num_bad);
    
    // For each bad state, add it as an output
    for (unsigned i = 0; i < model->num_bad; i++) {
        unsigned bad_literal = model->bad[i].lit;
        const char *bad_name = model->bad[i].name;
        
        // Add as output with the original name or a default name
        aiger_add_output(model, bad_literal, bad_name ? bad_name : "bad_state");
        printf("Added bad state %u as output\n", bad_literal);
    }
    
    // Clear all bad states
    unsigned num_bad = model->num_bad;
    for (unsigned i = 0; i < num_bad; i++) {
        // Remove the last bad state each time
        // We can't directly set num_bad=0 because we need to free any names
        // that might be allocated
        model->num_bad--;
    }
    
    printf("Modified model stats: M=%u, I=%u, L=%u, O=%u, A=%u, B=%u, C=%u\n",
           model->maxvar, model->num_inputs, model->num_latches,
           model->num_outputs, model->num_ands, model->num_bad, model->num_constraints);
    
    // Write the modified model using appropriate function for the format
    int write_success = 0;
    
    if (is_ascii_output) {
        FILE *file = fopen(output_filename, "w");
        if (!file) {
            perror("Failed to open output file");
            aiger_reset(model);
            return 1;
        }
        printf("Writing ASCII AIGER model...\n");
        write_success = aiger_write_to_file(model, aiger_ascii_mode, file);
        fclose(file);
    } else {
        printf("Writing binary AIGER model...\n");
        write_success = aiger_open_and_write_to_file(model, output_filename);
    }
    
    if (!write_success) {
        fprintf(stderr, "Failed to write AIGER model to %s\n", output_filename);
        aiger_reset(model);
        return 1;
    }
    
    printf("Successfully wrote modified model to %s\n", output_filename);
    
    // Clean up
    aiger_reset(model);
    return 0;
}
