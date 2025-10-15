#include "ns3/vector.h"

#include <tuple>
#include <vector>

using namespace ns3;

/**
 * Dataset CSV file.
 */
class DatasetCsv
{
public:
  /**
   * Constructor.
   */
  DatasetCsv() = default;

  /**
   * Get the number of CSV rows.
   *
   * @return Number of CSV rows.
   */
  std::size_t GetNRows() const;

  /**
   * Get the simulation time in seconds.
   *
   * @return Maximum time in seconds.
   */
  double GetMaxTimeS() const;

  /**
   * Get a row of the CSV dataset.
   *
   * @param csvRowIndex Index of the CSV row.
   * @return Tuple: [Tx position, Rx position]
   */
  std::tuple<Vector, Vector> GetCsvRow(std::size_t csvRowIndex) const;

  /**
   * Load the dataset CSV file.
   *
   * @param filename Dataset filename.
   * @return A vector of positions and respective average throughput.
   */
  void LoadMlplDatasetCsv(const std::string &filename);

  void LoadTraceBasedDatasetCsv(const std::string &filename);

private:
  std::vector<Vector> txPositions; //!< Vector of Tx positions
  std::vector<Vector> rxPositions; //!< Vector of Rx positions

  double m_maxTimeS = 0; //!< Time in seconds
};
