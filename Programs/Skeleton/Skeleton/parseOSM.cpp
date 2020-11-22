#include "parseOSM.hpp"
#define _CRT_SECURE_NO_WARNINGS


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



	char instr[250] = {};

	// Get bounds --------
	bool searchBounds = true;
	while (searchBounds)
		if (0 < sscanf_s(instr, R"( <bounds minlat="%lf" minlon="%lf" maxlat="%lf" maxlon="%lf)",
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

			success = 0 < sscanf_s(instr, " <node id=\"%lli\" %*s %*s lat=\"%lf\" lon=\"%lf", &id, &lat, &lon);

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
	auto pushTag = [&isCached, &wayType](const char* instr)
	{
		bool success;

		if (isCached)
		{
			return false;
		}
		else
		{
			char attr_name[300]{};
			char attr_val[300]{};

			success = 1 < sscanf_s(instr, "  <tag k=\"%[^\"]\" v=\"%[^\"]", &attr_name, (unsigned int)std::size(attr_name), &attr_val, (unsigned int)std::size(attr_val));

			if (success)
			{
				std::string sattr_name = attr_name;
				std::string sattr_val = attr_val;
				//we found a way tag

				auto getTypeIter = attr_val_to_EWayType.find(sattr_val);
				if (getTypeIter != attr_val_to_EWayType.end())
					wayType = getTypeIter->second;
				else
				{
					auto getTypeIter = attr_name_to_EWayType.find(sattr_name);
					if (getTypeIter != attr_name_to_EWayType.end())
						wayType = getTypeIter->second;
				}
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

			bool success = 0 < sscanf_s(instr, "t %lf %lf", &lat, &lon);

			if (success)
			{
				way.nodes.push_back({ lat, lon });
			}
			return success;
		}
		else
		{
			long long id_ref = 0;

			bool success = 0 < sscanf_s(instr, "  <nd ref=\"%lli", &id_ref);

			if (success)
			{
				//we found a way reference to node
				auto iter = nodesParam.find(id_ref);
				if (iter != nodesParam.end())
				{
					way.nodes.push_back(iter->second);
				}
			}

			return success;
		}
	};
	auto pushWay = [&way, &isCached, &mapData, &wayType](const char* instr)
	{
		if (isCached)
		{
			char name[50]{};
			int type;

			bool success = 0 < sscanf_s(instr, "w %s %i", name, (unsigned int)std::size(name), &type);

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
			bool success = 0 < sscanf_s(instr, " <w%c", &garbage, 1);

			if (success)
			{
				//we found a way
				mapData.push_back(wayType, way);
				way = Way();
				wayType = EWayType::defaultt;
			}

			return success;
		}

	};

	std::cout << "Now parsing nodes...\n";

	while (filestream)
	{
		filestream.getline(instr, sizeof(instr));

		if (filestream.gcount() == (sizeof(instr) - 1))
			filestream.clear();

		if (pushNode(instr))
			;
		else if (pushTag(instr))
			;
		else if (pushRef(instr))
			;
		else 
			pushWay(instr);
	}


	// Return --------
	//  Statistics ----
	auto end = std::chrono::steady_clock::now();
	auto timeinterval = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() +1;
	auto size = isCached ? std::filesystem::file_size(path + ".cache") : std::filesystem::file_size(path);
	auto speed = size / 1024 / 1024 / timeinterval;
	std::cout << "Finished parsing nodes (in " << timeinterval << " seconds) "
		<< speed << "MB/s\n";


	std::cout << "Found: " << mapData.getNodeCount() << " Nodes, " << mapData.getWayCount() << " Ways\n";


	// Generate cache
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
				filestream2 << "w " << way2.name << " " << (int)wayArr.first << "\n";
				for (const auto& attr2 : way2.nodes)
					filestream2 << "t " 
					<< std::setprecision(9) << attr2.lat << " " 
					<< std::setprecision(9) << attr2.lon << "\n";
			}
		}
		filestream2.close();
	}


	return true;
}

bool getNodesFromXML(const std::string& xml, MapData& mapData) 
{
	pugi::xml_document doc;
	doc.load_string(xml.c_str());

	auto e_osm = doc.child("osm");

	nodemap nodes;
	waymap ways;


	// Process nodes
	for (const auto& i : e_osm.children("node"))
		nodes.insert(nodemap_entry{ atoll(i.attribute("id").value()), NodeStripped{atof(i.attribute("lat").value()),atof(i.attribute("lon").value())} });

	// Process ways
	for (const auto& e_way : e_osm.children("way"))
	{
		Way way;
		way.name = "Q";
		EWayType wayType = EWayType::defaultt;
		
		// Fill the Way nodes
		for (const auto& e_nd : e_way.children("nd"))
		{
			long long refd_node = atoll(e_nd.attribute("ref").value());

			auto iter = nodes.find(refd_node);
			if (iter != nodes.end())
				way.nodes.push_back(iter->second);
		}

		// Get WayType
		for (const auto& e_tag : e_way.children("tag"))
		{
			std::string sattr_name = e_tag.attribute("k").value();
			std::string sattr_val = e_tag.attribute("v").value();

			auto getTypeIter = attr_val_to_EWayType.find(sattr_val);
			if (getTypeIter != attr_val_to_EWayType.end())
			{
				wayType = getTypeIter->second;
				break;
			}
			else
			{
				auto getTypeIter = attr_name_to_EWayType.find(sattr_name);
				if (getTypeIter != attr_name_to_EWayType.end())
				{
					wayType = getTypeIter->second;
					break;
				}
			}
		}

		mapData.push_back(wayType, way);
	}


	std::cout << "Parsed " << mapData.getNodeCount() << " nodes from received XML\n";
	std::cout << "       " << mapData.getWayCount() << " ways\n";
	//std::cout << "       " << mapData.getNodeCount(EWayType::boundary) << " boundary nodes\n";


	return true;
}

std::string* getOnlineOSM(const Bounds& bounds)
{
	char request[300]{};

	sprintf_s(request, std::size(request),
R"(data=<union>
  <bbox-query s="%f" w="%f" n="%f" e="%f"/>
  <recurse type="node-way"/>
</union>
<print/>)", bounds.minlat, bounds.minlon, bounds.maxlat, bounds.maxlon);

	std::cout << "\n---------------------\n" << request << "\n-----------------------\n";

	CURL* curl;
	CURLcode res;

	std::string* readBuffer = new std::string;

	/* get a curl handle */
	curl = curl_easy_init();
	if (curl) {
		/* First set the URL that is about to receive our POST. This URL can
		   just as well be a https:// URL if that is what should receive the
		   data. */
		curl_easy_setopt(curl, CURLOPT_URL, "https://lz4.overpass-api.de/api/interpreter");
		/* Now specify the POST data */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);


		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, readBuffer);


		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));


		/* always cleanup */
		curl_easy_cleanup(curl);

		std::cout << "\n---------------------\n" << readBuffer->substr(0,500) << "\n-----------------------\n";
	}


	return readBuffer;
}

size_t WriteCallback(char* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}