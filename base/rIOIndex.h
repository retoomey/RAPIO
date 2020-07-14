#pragma once

#include <rIndexType.h>
#include <rURL.h>
#include <vector>

namespace rapio {
/**
 * Provides a way to create an appropriate index object
 */
class IOIndex : public IO {
public:

  /**
   * Pass in a complete url, with the protocol used.
   *
   * By providing ActionListeners at construction, you can
   * create an index for which the event notification happens
   * even for the initial records.
   *
   */
  static std::shared_ptr<IndexType>
  createIndex(const std::string & protocol,
    const std::string           & indexparams,
    const TimeDuration          & maximumHistory =
    TimeDuration());

  /** Destroy an index */
  virtual
  ~IOIndex();

  /**
   * Returns a URL fragment that specifies a file's location.
   * It <b>must</b> contain the scheme, host, and directory portion of path.
   * It <b>may</b> contain user, password, port, query, and fragment.
   *
   * The URL fragment is returned as a string because it is intended for
   * the parameters string vector.
   *
   * <b>Warning:</b> Index path substitution is done via a simple string
   * comparison,
   * so beware of extraneous slashes, "localhost" vs. local host's name,
   * implict vs. explicit port numbers, etc.
   *
   * <b>Warning:</b> Care must be taken by Builders when constructing filenames
   * with
   * an indexPath. With our earlier notation
   *("xmllb:horus:/data/code_index.lb"),
   *  we could just concatenate indexPath and a relativePath to make a filename.
   * Since URLs place their query and fragment sections <i>after</i> the path,
   * this is no longer possible.  Instead, construct a URL object from
   * indexPath,
   * then append relativePath to the end of url.path:
   * <blockquote><pre>
   * rapio::URL url = indexPath;
   * url.path += "/" + rest_of_path;
   * </pre></blockquote>
   *
   * How an indexPath is generated:
   * <ol>
   * <li>If host is empty, it's set to the localhost.
   * <li>If path.getDirName() is empty, it's set to the current working
   * directory.
   * <li>path's basename is stripped out. (url.path = url.getDirName())
   * <li>Substitutions from misc/directoryMapping.xml are performed.
   * </ol>
   */
  static std::string
  getIndexPath(const URL& file);

public:

  // Registering of classes ---------------------------------------------

  /** Introduce self into factories */
  static void
  introduce(const std::string  & protocol,
    std::shared_ptr<IndexType> factory);

  /** Use this to introduce new subclasses. */
  static void
  introduceSelf();
}
;
}
