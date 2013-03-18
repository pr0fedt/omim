#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile_cache.hpp"
#include "tile_set.hpp"
#include "drawer.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../base/commands_queue.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/vector.hpp"

namespace graphics
{
  class ResourceManager;
  class PacketsQueue;

  namespace gl
  {
    class RenderContext;
  }
}

class WindowHandle;
class Drawer;

class TileRenderer
{
protected:

  core::CommandsQueue m_queue;

  shared_ptr<graphics::ResourceManager> m_resourceManager;

  struct ThreadData
  {
    Drawer * m_drawer;
    Drawer::Params m_drawerParams;
    unsigned m_threadSlot;
    shared_ptr<graphics::gl::BaseTexture> m_dummyRT;
    shared_ptr<graphics::RenderContext> m_renderContext;
    shared_ptr<graphics::gl::RenderBuffer> m_depthBuffer;
  };

  buffer_vector<ThreadData, 4> m_threadData;

  shared_ptr<graphics::RenderContext> m_primaryContext;

  TileCache m_tileCache;

  /// set of already rendered tiles, which are waiting
  /// for the CoverageGenerator to process them
  TileSet m_tileSet;

  size_t m_tileSize;

  RenderPolicy::TRenderFn m_renderFn;
  string m_skinName;
  graphics::EDensity m_density;
  graphics::Color m_bgColor;
  int m_sequenceID;
  bool m_isExiting;

  bool m_isPaused;

  threads::Mutex m_tilesInProgressMutex;
  set<Tiler::RectInfo> m_tilesInProgress;

  void InitializeThreadGL(core::CommandsQueue::Environment const & env);
  void FinalizeThreadGL(core::CommandsQueue::Environment const & env);

protected:

  virtual void DrawTile(core::CommandsQueue::Environment const & env,
                        Tiler::RectInfo const & rectInfo,
                        int sequenceID);

  void ReadPixels(graphics::PacketsQueue * glQueue, core::CommandsQueue::Environment const & env);

public:

  /// constructor.
  TileRenderer(size_t tileSize,
               string const & skinName,
               graphics::EDensity density,
               unsigned tasksCount,
               graphics::Color const & bgColor,
               RenderPolicy::TRenderFn const & renderFn,
               shared_ptr<graphics::RenderContext> const & primaryRC,
               shared_ptr<graphics::ResourceManager> const & rm,
               double visualScale,
               graphics::PacketsQueue ** packetsQueue);
  /// destructor.
  virtual ~TileRenderer();
  /// add command to the commands queue.
  void AddCommand(Tiler::RectInfo const & rectInfo,
                  int sequenceID,
                  core::CommandsQueue::Chain const & afterTileFns = core::CommandsQueue::Chain());
  /// get tile cache.
  TileCache & GetTileCache();
  /// get tile from render if tile already rendered, otherwise return NULL
  Tile const * GetTile(Tiler::RectInfo const & rectInfo);
  /// wait on a condition variable for an empty queue.
  void WaitForEmptyAndFinished();

  void SetSequenceID(int sequenceID);

  void CancelCommands();

  void ClearCommands();

  bool HasTile(Tiler::RectInfo const & rectInfo);

  /// add tile to the temporary set of rendered tiles, cache it and lock it in the cache.
  /// temporary set is necessary to carry the state between corresponding Tile rendering
  /// commands and MergeTile commands.
  void AddActiveTile(Tile const & tile);
  /// remove tile from the TileSet.
  /// @param doUpdateCache shows, whether we should
  void RemoveActiveTile(Tiler::RectInfo const & rectInfo, int sequenceID);

  void SetIsPaused(bool flag);

  size_t TileSize() const;
};
