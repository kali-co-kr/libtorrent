#ifndef LIBTORRENT_DOWNLOAD_STATE_H
#define LIBTORRENT_DOWNLOAD_STATE_H

#include <sigc++/signal.h>

#include "content/content.h"
#include "data/hash_torrent.h"
#include "torrent/peer.h"

#include "bitfield_counter.h"
#include "peer_info.h"

namespace torrent {

extern HashQueue hashQueue;
extern HashTorrent hashTorrent;

class DownloadSettings;

// Here goes all those things that Peer* and Delegator needs.
class DownloadState {
public:
  DownloadState();
  ~DownloadState();

  PeerInfo&         get_me()                                 { return m_me; }
  std::string&      get_hash()                               { return m_hash; }

  DownloadSettings& get_settings()                           { return *m_settings; }
  void              set_settings(DownloadSettings* s)        { m_settings = s; }

  Content&          get_content()                            { return m_content; }
  BitFieldCounter&  get_bitfield_counter()                   { return m_bfCounter; }

  void              chunk_done(unsigned int index);

  uint64_t          bytes_left();

  // Set endgame mode if we're close enough to the end.
  void              update_endgame();

  void              receive_hashdone(Storage::Chunk c, std::string hash);

  typedef sigc::signal1<void, uint32_t> SignalChunk;
  typedef sigc::slot1<void, bool>       SlotSetEndgame;
  typedef sigc::slot0<uint32_t>         SlotDelegatedChunks;

  SignalChunk&                          signal_chunk_passed()      { return m_signalChunkPassed; }
  SignalChunk&                          signal_chunk_failed()      { return m_signalChunkFailed; }

  void slot_set_endgame(SlotSetEndgame s)                          { m_slotSetEndgame = s; }
  void slot_delegated_chunks(SlotDelegatedChunks s)                { m_slotDelegatedChunks = s; }

private:
  // Disable copy ctor and assignment
  DownloadState(const DownloadState&);
  void operator = (const DownloadState&);

  PeerInfo            m_me;
  std::string         m_hash;
  
  DownloadSettings*   m_settings;
  Content             m_content;
  BitFieldCounter     m_bfCounter;

  SignalChunk         m_signalChunkPassed;
  SignalChunk         m_signalChunkFailed;

  SlotSetEndgame      m_slotSetEndgame;
  SlotDelegatedChunks m_slotDelegatedChunks;
};

}

#endif