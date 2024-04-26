/************************************************************************************\
 *                                                                                  *
 * Copyright (c) 2024 James Braun and Matthew D. Sinclair                           *
 * All rights reserved.                                                             *
 *                                                                                  *
 * Redistribution and use in source and binary forms, with or without               *
 * modification, are permitted provided that the following are met:                 *
 *                                                                                  *
 * You must reproduce the above copyright notice.                                   *
 *                                                                                  *
 * Neither the name of the copyright holder nor the names of its contributors       *
 * may be used to endorse or promote products derived from this software            *
 * without specific, prior, written permission from at least the copyright holder.  *
 *                                                                                  *
 * You must include the following terms in your license and/or other materials      *
 * provided with the software.                                                      *
 *                                                                                  *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"      *
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE        *
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, AND FITNESS FOR A       *
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER        *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,         *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS      *
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN          *
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  *
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY   *
 * OF SUCH DAMAGE.                                                                  *
 *                                                                                  *
 * Without limiting the foregoing, the software may implement third party           *
 * technologies for which you must obtain licenses from parties other than AMD.     *
 * You agree that AMD has not obtained or conveyed to you, and that you shall       *
 * be responsible for obtaining the rights to use and/or distribute the applicable  *
 * underlying intellectual property rights related to the third party technologies. *
 * These third party technologies are not licensed hereunder.                       *
 *                                                                                  *
 * If you use the software (in whole or in part), you shall adhere to all           *
 * applicable U.S., European, and other export laws, including but not limited to   *
 * the U.S. Export Administration Regulations ("EAR") (15 C.F.R Sections 730-774),  *
 * and E.U. Council Regulation (EC) No 428/2009 of 5 May 2009.  Further, pursuant   *
 * to Section 740.6 of the EAR, you hereby certify that, except pursuant to a       *
 * license granted by the United States Department of Commerce Bureau of Industry   *
 * and Security or as otherwise permitted pursuant to a License Exception under     *
 * the U.S. Export Administration Regulations ("EAR"), you will not (1) export,     *
 * re-export or release to a national of a country in Country Groups D:1, E:1 or    *
 * E:2 any restricted technology, software, or source code you receive hereunder,   *
 * or (2) export to Country Groups D:1, E:1 or E:2 the direct product of such       *
 * technology or software, if such foreign produced direct product is subject to    *
 * national security controls as identified on the Commerce Control List (currently *
 * found in Supplement 1 to Part 774 of EAR).  For the most current Country Group   *
 * listings, or for additional information about the EAR or your obligations under  *
 * those regulations, please refer to the U.S. Bureau of Industry and Security's    *
 * website at http://www.bis.doc.gov/.                                              *
 *                                                                                  *
\************************************************************************************/

#ifndef PAGERANK_PARSE_H
#define PAGERANK_PARSE_H

#include "../graph_parser/parse.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void use_pagerank_mmap(csr_array *csr, int *num_nodes, int *num_edges)
{
    printf("Using an mmap!\n");

    // get num_nodes
    int fd = open("pagerank_row_mmap.bin", std::ios::binary | std::fstream::in);
    if (fd == -1) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        fprintf(stderr, "You need to create an mmapped input file! pagerank_row_mmap.bin is missing!\n");
        exit(1);
    }

    int offset = 0;
    num_nodes = *((int *)mmap(NULL, 1 * sizeof(int), PROT_READ, MAP_PRIVATE, fd, offset));

    // read row_array in
    int *row_array_map = (int *)mmap(NULL, (num_nodes + 2) * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, offset);

    // Check that maping was sucessful
    if (row_array_map == MAP_FAILED) {
        fprintf(stderr, "row mmap failed!\n");
        exit(1);
    }

    // Copy row_array
    csr = (csr_array *)malloc(sizeof(csr_array));
    if (csr == NULL) {
        printf("csr_array malloc failed!\n");
        exit(1);
    }

    int *row_array = (int *)malloc((num_nodes + 1) * sizeof(int));
    memcpy(row_array, &row_array_map[1], (num_nodes + 1) * sizeof(int));

    munmap(row_array_map, (num_nodes + 2) * sizeof(int));
    close(fd);

    // get num_edges
    fd = open("pagerank_col_mmap.bin", std::ios::binary | std::fstream::in);
    if (fd == -1) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        fprintf(stderr, "You need to create an mmapped input file! pagerank_col_mmap.bin is missing!\n");
        exit(1);
    }

    offset = 0;
    num_edges = *((int *)mmap(NULL, 1 * sizeof(int), PROT_READ, MAP_PRIVATE, fd, offset));

    // read col_array in
    int *col_array_map = (int *)mmap(NULL, (num_edges + 1) * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, offset);

    // Check that maping was sucessful
    if (col_array_map == MAP_FAILED) {
        fprintf(stderr, "mmap failed!\n");
        exit(1);
    }

    // Copy col_array
    int *col_array = (int *)malloc(num_edges * sizeof(int));
    memcpy(col_array, &col_array_map[1], num_edges * sizeof(int));

    munmap(col_array_map, (num_edges + 1) * sizeof(int));
    close(fd);

    // open col_cnt binary
    fd = open("pagerank_col_cnt_mmap.bin", std::ios::binary | std::fstream::in);
    if (fd == -1) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        fprintf(stderr, "You need to create an mmapped input file! pagerank_col_cnt_mmap.bin is missing!\n");
        exit(1);
    }

    // read col_cnt_array in
    offset = 0;
    int *col_cnt_map = (int *)mmap(NULL, num_nodes * sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, offset);

    // Check that maping was sucessful
    if (col_cnt_map == MAP_FAILED) {
        fprintf(stderr, "mmap failed!\n");
        exit(1);
    }

    // Copy col_cnt_array
    int *col_cnt_array = (int *)malloc(num_nodes * sizeof(int));
    memcpy(col_cnt_array, col_cnt_map, num_nodes * sizeof(int));

    munmap(col_cnt_map, num_nodes * sizeof(int));
    close(fd);

    memset(csr, 0, sizeof(csr_array));
    csr->row_array = row_array;
    csr->col_array = col_array;
    csr->col_cnt = col_cnt_array;
}

void create_pagerank_mmap(csr_array *csr, int num_nodes, int num_edges)
{
     printf("creating an mmap\n");

     // prints csr to file
     std::ofstream row_out("pagerank_row_mmap.bin", std::ios::binary);

     row_out.write((char *)&num_nodes, sizeof(int));
     row_out.write((char *)csr->row_array, (num_nodes + 1) * sizeof(int));

     row_out.close();

     // num_edges * sizeof(int)
     std::ofstream col_out("pagerank_col_mmap.bin", std::ios::binary);

     col_out.write((char *)&num_edges, sizeof(int));
     col_out.write((char *)csr->col_array, num_edges * sizeof(int));

     col_out.close();

     std::ofstream col_cnt_out("pagerank_col_cnt_mmap.bin", std::ios::binary);

     col_cnt_out.write((char *)csr->col_cnt, num_nodes * sizeof(int));

     col_cnt_out.close();

     csr->freeArrays();
     free(csr);
     printf("mmaps created!\n");
     return 0;
}

#endif
