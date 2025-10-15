#include "dataset-csv.h"

#include "ns3/csv-reader.h"
#include "ns3/vector.h"

#include <tuple>
#include <vector>

using namespace ns3;

std::size_t
DatasetCsv::GetNRows() const
{
    return txPositions.size();
}

double
DatasetCsv::GetMaxTimeS() const
{
    return m_maxTimeS;
}

std::tuple<Vector, Vector>
DatasetCsv::GetCsvRow(std::size_t csvRowIndex) const
{
    if (csvRowIndex >= txPositions.size())
    {
        NS_ABORT_MSG("Trying to access ann invalid dataset CSV row (" << csvRowIndex << ")");
    }

    return {txPositions.at(csvRowIndex), rxPositions.at(csvRowIndex)};
}

void DatasetCsv::LoadMlplDatasetCsv(const std::string &filename)
{
    txPositions.clear();
    rxPositions.clear();

    CsvReader csvReader(filename);

    // Skip CSV header
    csvReader.FetchNextRow();

    while (csvReader.FetchNextRow())
    {
        if (csvReader.IsBlankRow())
        {
            continue;
        }

        // Read Tx position
        Vector txPosition;

        if (!csvReader.GetValue(0, txPosition.x))
        {
            NS_ABORT_MSG("Error reading field \"x_tx\" from the dataset CSV file");
        }
        if (!csvReader.GetValue(1, txPosition.y))
        {
            NS_ABORT_MSG("Error reading field \"y_tx\" from the dataset CSV file");
        }
        if (!csvReader.GetValue(2, txPosition.z))
        {
            NS_ABORT_MSG("Error reading field \"z_tx\" from the dataset CSV file");
        }

        txPositions.emplace_back(txPosition);

        // Read Rx position
        Vector rxPosition;

        if (!csvReader.GetValue(3, rxPosition.x))
        {
            NS_ABORT_MSG("Error reading field \"x_rx\" from the dataset CSV file");
        }
        if (!csvReader.GetValue(4, rxPosition.y))
        {
            NS_ABORT_MSG("Error reading field \"y_rx\" from the dataset CSV file");
        }
        if (!csvReader.GetValue(5, rxPosition.z))
        {
            NS_ABORT_MSG("Error reading field \"z_rx\" from the dataset CSV file");
        }

        rxPositions.emplace_back(rxPosition);
    }
}

void DatasetCsv::LoadTraceBasedDatasetCsv(const std::string &filename)
{
    CsvReader csvReader(filename);

    // Skip CSV header
    csvReader.FetchNextRow();

    while (csvReader.FetchNextRow())
    {
        if (csvReader.IsBlankRow())
        {
            continue;
        }

        double timeS;
        uint32_t txNode;
        uint32_t rxNode;
        double rxPowerDbm;

        if (!csvReader.GetValue(0, timeS))
        {
            NS_ABORT_MSG("Error reading field \"time_s\" from the network trace CSV file");
        }
        if (!csvReader.GetValue(1, txNode))
        {
            NS_ABORT_MSG("Error reading field \"tx_node\" from the network trace CSV file");
        }
        if (!csvReader.GetValue(2, rxNode))
        {
            NS_ABORT_MSG("Error reading field \"rx_node\" from the network trace CSV file");
        }
        if (!csvReader.GetValue(3, rxPowerDbm))
        {
            NS_ABORT_MSG("Error reading field \"rx_power_dbm\" from the network trace CSV file");
        }

        if (m_maxTimeS < timeS)
        {
            m_maxTimeS = timeS;
        }
    }
}
