#include <basic.h>
#include <kernel.h>
#include <trains/reservoir.h>
#include <track/pathing.h>

int reservoir_tid = -1;

typedef struct {
  // type = RESERVOIR_PATHING_REQUEST
  packet_t packet;
  int train;
  int src_node;
  int dest_node;
} pathing_request_t;

static inline int get_segment_owner(segment_t * segment) {
  if (track[segment->track_node].edge[segment->dir].owner != -1)
    return track[segment->track_node].edge[segment->dir].owner;
  else if (track[segment->track_node].edge[segment->dir].reverse->owner != -1)
    return track[segment->track_node].edge[segment->dir].reverse->owner;
  else
    return -1;
}

bool all_segments_available(reservoir_segments_t * request) {
  for (int i = 0; i < request->len; i++) {
    if (get_segment_owner(&request->segments[i]) != -1) return false;
  }
  return true;
}

void set_segment_owner(segment_t * segment, int owner) {
  track[segment->track_node].edge[segment->dir].owner = owner;
}

void set_segment_ownership(reservoir_segments_t * request, int owner) {
  for (int i = 0; i < request->len; i++) {
    set_segment_owner(&request->segments[i], owner);
  }
}

void release_segments(reservoir_segments_t * request, int owner) {
  for (int i = 0; i < request->len; i++) {
    KASSERT(get_segment_owner(&request->segments[i]) == owner, "Attempted to release segment not owned. node=%d dir=%d name=%s owner=%d actual_owner=%d",
      request->segments[i].track_node,
      request->segments[i].dir,
      track[request->segments[i].track_node].name,
      owner,
      get_segment_owner(&request->segments[i]));
    set_segment_owner(&request->segments[i], -1);
  }
}

static void process_pathing_request(path_t * path, pathing_request_t * request, int sender) {
  // FIXME: take into account train ownership
  GetPath(path, request->src_node, request->dest_node);
}

void reservoir_task() {
  int tid = MyTid();
  reservoir_tid = tid;

  char request_buffer[128];
  KASSERT(sizeof(request_buffer) >= sizeof(reservoir_segments_t), "Buffer isn't large enough.");
  KASSERT(sizeof(request_buffer) >= sizeof(pathing_request_t), "Buffer isn't large enough.");
  packet_t * packet = (packet_t *) request_buffer;
  reservoir_segments_t * resv_request = (reservoir_segments_t *) request_buffer;
  pathing_request_t * path_request = (pathing_request_t *) request_buffer;
  path_t result_path;

  int sender;
  while (true) {
    ReceiveS(&sender, request_buffer);
    switch (packet->type) {
    case RESERVOIR_REQUEST:
      if (all_segments_available(resv_request)) {
        set_segment_ownership(resv_request, sender);
        ReplyStatus(sender, RESERVOIR_REQUEST_OK);
      } else {
        ReplyStatus(sender, RESERVOIR_REQUEST_ERROR);
      }
      break;
    case RESERVOIR_RELEASE:
      release_segments(resv_request, sender);
      ReplyN(sender);
      break;
    case RESERVOIR_PATHING_REQUEST:
      process_pathing_request(&result_path, path_request, sender);
      // FIXME: if no path found due to ownership, ReplyN
      ReplyS(sender, result_path);
      break;
    }
  }
}


int RequestSegment(reservoir_segments_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->len > 0 && segment->len < 12, "Segment overflowed");
  // TODO: kassert valid segments and directions
  segment->packet.type = RESERVOIR_REQUEST;
  int result;
  Send(reservoir_tid, segment, sizeof(reservoir_segments_t), &result, sizeof(result));
  return result;
}

void ReleaseSegment(reservoir_segments_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->len > 0 && segment->len < 12, "Segment overflowed");
  // TODO: kassert valid segments and directions
  segment->packet.type = RESERVOIR_RELEASE;
  Send(reservoir_tid, segment, sizeof(reservoir_segments_t), NULL, 0);
}


int RequestPath(path_t * output, int train, int src_node, int dest_node) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  pathing_request_t request;
  request.packet.type = RESERVOIR_PATHING_REQUEST;
  request.train = train;
  request.src_node = src_node;
  request.dest_node = dest_node;
  Send(reservoir_tid, &request, sizeof(reservoir_segments_t), output, sizeof(path_t));
  // FIXME: get pathing result
  // maybe if 0 bytes written we've failed?
  return 0;
}