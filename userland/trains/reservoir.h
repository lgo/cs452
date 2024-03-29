#pragma once

/**
 * The Reservoir is for reserving segments of track.
 * This is used by train controlers in order to know where
 * trains can traverse
 */

#include <packet.h>
#include <track/track_node.h>
#include <track/pathing.h>

#define RESERVOIR_REQUEST_OK 0
#define RESERVOIR_REQUEST_ERROR 1

#define RESERVING_LIMIT 32

/**
 * segment_t specifies a track segment, i.e. edge, from a node and direction
 * TODO: maybe extend this to be either an edge or a node?
 */
typedef struct {
  // a track node
  int track_node;
  // either DIR_AHEAD
  // or DIR_CURVED and DIR_STRAIGHT
  unsigned char dir;
} segment_t;

typedef struct {
  // NOTE: for internal message passing, either
  // RESERVOIR_REQUEST or RESERVOIR_RELEASE
  // RESERVOIR_AND_RELEASE_REQUEST
  packet_t packet;

  int owner;
  // Reasonable upper limit to request or release at one time
  segment_t segments[RESERVING_LIMIT];
  int len;
} reservoir_segments_t;

typedef struct {
  int owner;
  // Reasonable upper limit to request or release at one time
  track_edge *edges[RESERVING_LIMIT];
  int len;
} reservoir_segment_edges_t;

void reservoir_task();

/**
 * Request ownership over segments
 * NOTE: blocking request (Send)
 * @param  segment to ask for (includes nodes and requested train)
 * @return         status of request
 *                   0 => OK
 *                   1 => FAILED (already owned)
 */
int RequestSegment(reservoir_segments_t * segment);

int RequestSegmentEdges(reservoir_segment_edges_t * segment);

int RequestSegmentEdgesAndReleaseRest(reservoir_segment_edges_t * segment);

/**
 * Release ownership over segments
 * NOTE: you must already own them to release them, otherwise this
 * NOTE: blocking request (Send)
 * causes a KASSERT
 * @param segment to release.
 */
void ReleaseSegment(reservoir_segments_t * segment);

void ReleaseSegmentEdges(reservoir_segment_edges_t * segment);

/**
 * Requests a path that has no owned segments
 * NOTE: blocking request (Send)
 * @param  output    to store pathing result
 * @param  train     that we want to check ownership with
 * @param  src_node  for the path
 * @param  dest_node for the path
 * @return           status of request
 *                     0 => OK
 *                     1 => FAILED (no direct path)
 */
int RequestPath(path_t * output, int train, int src_node, int dest_node);

/**
 * Gets the owner of a segment
 * @param  node to request
 * @return      owner
 *              if no owner, returns -1
 */
int WhoOwnsSegment(int node);
