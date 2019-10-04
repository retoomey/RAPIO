#pragma once

#include <rIndexType.h>
#include <rIOXML.h>

namespace rapio {
/**
 * The XMLIndex class implements the Index interface by obtaining
 * all the needed information from a XML file that follows certain
 * conventions. See the documentation for the method constructRecord
 * for the XML format. The XML file itself has this format:
 * <pre>
 *   <codeindex>
 *     <item> ... </item>
 *     .
 *     .
 *     .
 *     <item> ... </item>
 *   </codeindex>
 * </pre>
 * with each of the item tags corresponding to an Record.
 *
 * @see constructRecord
 */
class XMLIndex : public IndexType {
public:

  // Factory
  // ---------------------------------------------------------------------

  /** Empty for factory */
  XMLIndex(){ }

  /** Introduce to index builder */
  static void
  introduceSelf();

  /** Create a XML index */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const URL                                    & location,
    std::vector<std::shared_ptr<IndexListener> > listeners,
    const TimeDuration                           & maximumHistory) override;

  /**
   * Parse the given XML file for the information.
   *
   * This will ensure that all the records are added.
   * Hence, this index will become searchable.
   *
   * @param xmlFile the file to parse
   * @param baseClass (optional) -- use for simulations, etc.
   */
  XMLIndex(const URL                                  & xmlFile,
    const std::vector<std::shared_ptr<IndexListener> >& listeners,
    const TimeDuration                                & maximumHistory);

  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime) override;

  virtual
  ~XMLIndex();

  bool
  isValid() const
  {
    return (readok);
  }

public:

  /** Extract out of this param element, placing the stuff into params.
   *  If necessary use the previous params in the vector. */
  bool readok;
};
}
