#include "parseOSM.hpp"


// Extract data from osm xml
bool getNodes(const std::string& path, MapData& mapData)
{
	auto begin = std::chrono::steady_clock::now();

	nodemap nodesParam;
	Way way = Way();
	EWayType wayType = EWayType::defaultt;


	// Getting real path 
	bool isCached = std::filesystem::exists(path + ".cache");
	const std::string realPath = isCached? path + ".cache" : path;

	std::cout << "Found cache? " << (isCached ? "Yes\n" : "No\n");

	std::ifstream filestream(realPath);

	std::cout << "Opened file at " << realPath << "\n";



	char instr[250];

	// Get bounds --------
	bool searchBounds = true;
	while (searchBounds)
		if (0 < sscanf(instr, " <bounds minlat=\"%lf\" minlon=\"%lf\" maxlat=\"%lf\" maxlon=\"%lf",
			&mapData.bounds.minlat,
			&mapData.bounds.minlon,
			&mapData.bounds.maxlat,
			&mapData.bounds.maxlon))
		{
			searchBounds = false;
			std::cout << "Found bounds: " 
				<< mapData.bounds.minlat << " " 
				<< mapData.bounds.maxlat << " " 
				<< mapData.bounds.minlon << " " 
				<< mapData.bounds.maxlon << "\n";
		}
		else
		{
			filestream.getline(instr, sizeof(instr));

			if (filestream.gcount() == (sizeof(instr) - 1))
				filestream.clear();
		}


	// Parse File --------
	auto pushNode = [&nodesParam, &isCached](const char* instr)
	{
		bool success;

		if (isCached)
		{
			return false;
		}
		else
		{
			double lat = 0;
			double lon = 0;
			long long id = 0;

			success = 0 < sscanf(instr, " <node id=\"%lli\" %*s %*s lat=\"%lf\" lon=\"%lf", &id, &lat, &lon);

			if (success)
				//we found a node
				if (lat != 0 && lon != 0 && id > 0)
				{
					nodemap_entry map_entry = nodemap_entry(id, { lat, lon });
					nodesParam.insert(nodemap_entry(map_entry));
				}

			return success;
		}

	};
	auto pushTag = [&way, &isCached, &wayType](const char* instr)
	{
		bool success;

		if (isCached)
		{
			return false;
		}
		else
		{
			char attr_name[300];
			char attr_val[300];

			success = 0 < sscanf(instr, "  <tag k=\"%[^\"]\" v=\"%[^\"]", &attr_name, &attr_val);

			if (success)
			{
				std::string sattr_name = attr_name;
				//we found a way tag

				auto getTypeIter = stringToEWayType.find(sattr_name);
				if(getTypeIter != stringToEWayType.end())
					wayType = getTypeIter->second;
				/*
				if (sattr_name == "building")
				{
					way.type = Way::EType::building;
				}
				else if (sattr_name == "boundary")
				{
					way.type = Way::EType::boundary;
				}
				else if (sattr_name == "highway")
				{
					way.type = Way::EType::route_highway;
				}
				else if (sattr_name == "railway")
				{
					way.type = Way::EType::route_railway;
				}
				*/
			}

			return success;
		}

	};
	auto pushRef = [&way, &isCached, &nodesParam](const char* instr)
	{
		if (isCached)
		{
			double lat = 0;
			double lon = 0;

			bool success = 0 < sscanf(instr, "t %lf %lf", &lat, &lon);

			if (success)
			{
				way.ids.push_back({ lat, lon });
			}
			return success;
		}
		else
		{
			long long id_ref = 0;

			bool success = 0 < sscanf(instr, "  <nd ref=\"%lli", &id_ref);

			if (success)
			{
				//we found a way reference to node
				auto iter = nodesParam.find(id_ref);
				if (iter != nodesParam.end())
				{
					way.ids.push_back(iter->second);
				}
			}

			return success;
		}
	};
	auto pushWay = [&way, &isCached, &mapData, &wayType](const char* instr)
	{
		if (isCached)
		{
			char name[50];
			int type;

			bool success = 0 < sscanf(instr, "w %s %i", &name, &type);

			if (success)
			{
				//we found a way
				mapData.push_back(wayType, way);
				way = Way();
				way.name = name;
				wayType = (EWayType)type;
			}

			return success;
		}
		else
		{
			char garbage;
			bool success = 0 < sscanf(instr, " <w%c", &garbage);

			if (success)
			{
				//we found a way
				mapData.push_back(wayType, way);
				way = Way();
			}

			return success;
		}

	};

	std::cout << "Now parsing nodes...\n";

	while (filestream)
	{
		long long id;
		char name[100];
		float lat = 0;
		float lon = 0;
		char attr_name[250];
		char attr_val[250];
		long long id_ref;

		filestream.getline(instr, sizeof(instr));

		if (filestream.gcount() == (sizeof(instr) - 1))
			filestream.clear();

		// Exported:               " <node id=\"%lli\" %*s %*s %*s %*s %*s %*s lat=\"%f\" lon=\"%f\""
		if (pushNode(instr))
			;
		else if (pushTag(instr))
			;
		else if (pushRef(instr))
			;
		else if (pushWay(instr))
			;
	}


	// Return --------
	//  Statistics ----
	auto end = std::chrono::steady_clock::now();
	auto timeinterval = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
	auto size = isCached ? std::filesystem::file_size(path + ".cache") : std::filesystem::file_size(path);
	auto speed = size / 1024 / 1024 / timeinterval;
	std::cout << "Finished parsing nodes (in " << timeinterval << " seconds) "
		<< speed << "MB/s\n";


	std::cout << "Found: " << mapData.getNodeCount() << " Nodes, " << mapData.getWayCount() << " Ways\n";


	if (!isCached)
	{
		std::cout << "Now writing cache to " << path + ".cache" << "\n";

		std::ofstream filestream2(path + ".cache");

		filestream2
			<< " <bounds minlat=\"" << mapData.bounds.minlat
			<< "\" minlon=\"" << mapData.bounds.minlon
			<< "\" maxlat=\"" << mapData.bounds.maxlat
			<< "\" maxlon=\"" << mapData.bounds.maxlon
			<< "\n";

		for (const auto& wayArr : mapData.wayArrs)
		{
			for (const auto& way2 : wayArr.second)
			{
				filestream2 << "w " << way2.name << " " << wayArr.first << "\n";
				for (const auto& attr2 : way2.ids)
					filestream2 << "t " 
					<< std::setprecision(9) << attr2.lat << " " 
					<< std::setprecision(9) << attr2.lon << "\n";
			}
		}
		filestream2.close();
	}


	return true;
}