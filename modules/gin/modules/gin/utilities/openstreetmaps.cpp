/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

OpenStreetMaps::OpenStreetMaps()
  : tileSource (OpenStreetMap)
{
    mapTileDir = File::getSpecialLocation (File::tempDirectory).getChildFile ("mapTiles");
    mapTileDir.createDirectory();
}

OpenStreetMaps::~OpenStreetMaps()
{
    requests.clear();
    cancelledRequests.clear();
}

Image OpenStreetMaps::fetchTile (int zoom, int x, int y)
{
    int mw = getMapWidthTiles (zoom);
    x = x % mw;
    y = y % mw;

    String fname = String::formatted ("%d-%d-%d-%d.png", (int)tileSource, zoom, x, y);

    File file = mapTileDir.getChildFile (fname);

    if (cache.contains (fname))
    {
        return cache[fname];
    }
    else if (File (file).existsAsFile())
    {
        Image img;
        img = ImageFileFormat::loadFrom (file);

        cache.set (fname, img);

        return img;
    }
    else
    {
        auto newReq = std::make_unique<TileReq> (zoom, x, y);

        bool pending = false;
        for (int i = 0; i < requests.size(); i++)
        {
            if (*requests[i] == *newReq)
            {
                pending = true;
                break;
            }
        }

        if (! pending)
        {
            requests.add (newReq.release());
            startRequest();
        }

        Image img (Image::ARGB, 256, 256, false);
        img.clear ({0,0,256,256}, Colour (0xff808080));
        return img;
    }
}

int OpenStreetMaps::getNumServers()
{
    switch (tileSource)
    {
        case OpenStreetMap:         return 3;
        case OpenCycleMap:          return 3;
        case OpenCycleMapTransport: return 3;
        case OpenCycleMapLandscape: return 3;
        case StamenTerrain:         return 1;
        case MapQuestOSM:           return 4;
        case MapQuestOpenAerial:    return 4;
        case MapQuestOpenStreetMap: return 3;
        default:                    return 0;
    }
}

int OpenStreetMaps::getServer()
{
    for (int i = 0; i < getNumServers(); i++)
    {
        if (! serversInUse.contains(i))
            return i;
    }
    return -1;
}

void OpenStreetMaps::startRequest()
{
    int server = getServer();
    if (server == -1)
        return;

    for (int i = 0; i < requests.size(); i++)
    {
        if (requests[i]->reply == nullptr)
        {
            char buffer[1024];
            switch (tileSource)
            {
                case OpenStreetMap:
                    sprintf(buffer, "http://%c.tile.openstreetmap.org/%d/%d/%d.png", "abc"[server], requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                case OpenCycleMap:
                    sprintf(buffer, "http://%c.tile.opencyclemap.org/cycle/%d/%d/%d.png", "abc"[server], requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                case OpenCycleMapTransport:
                    sprintf(buffer, "http://%c.tile2.opencyclemap.org/transport/%d/%d/%d.png", "abc"[server], requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                case OpenCycleMapLandscape:
                    sprintf(buffer, "http://%c.tile3.opencyclemap.org/landscape/%d/%d/%d.png", "abc"[server], requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                case StamenTerrain:
                    sprintf(buffer, "http://tile.stamen.com/terrain/%d/%d/%d.png", requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                case MapQuestOSM:
                    sprintf(buffer, "http://otile%c.mqcdn.com/tiles/1.0.0/map/%d/%d/%d.jpg", "1234"[server], requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                case MapQuestOpenAerial:
                    sprintf(buffer, "http://otile%c.mqcdn.com/tiles/1.0.0/sat/%d/%d/%d.jpg", "1234"[server], requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                case MapQuestOpenStreetMap:
                    sprintf(buffer, "http://%c.tile.openstreetmap.org/%d/%d/%d.png", "abc"[server], requests[i]->zoom, requests[i]->x, requests[i]->y);
                    break;
                default:
                    jassertfalse;
                    break;
            }

            requests[i]->server = server;

            serversInUse.add (server);

            URL url = URL (String (buffer));

            requests[i]->reply = std::make_unique<AsyncDownload> (buffer, [this] (AsyncDownload* ad, juce::MemoryBlock m, bool ok)
                                                    {
                                                        finished (ad, m, ok);
                                                    });

            break;
        }
    }
}

void OpenStreetMaps::finished (AsyncDownload* reply, juce::MemoryBlock data, bool success)
{
    for (int i = 0; i < requests.size(); i++)
    {
        if (requests[i]->reply.get() == reply)
        {
            serversInUse.removeFirstMatchingValue (requests[i]->server);
            requests[i]->server = -1;

            if (success)
            {
                String fname;
                fname = String::formatted ("%d-%d-%d-%d.png", (int)tileSource, requests[i]->zoom, requests[i]->x, requests[i]->y);

                Image img = ImageFileFormat::loadFrom (data.getData(), data.getSize());
                if (img.isValid())
                {
                    cache.set (fname, img);

                    File dest = mapTileDir.getChildFile (fname);
                    dest.replaceWithData (data.getData(), data.getSize());

                    listeners.call (&OpenStreetMaps::Listener::tileFetched, requests[i]->zoom, requests[i]->x, requests[i]->y);
                }

            }
            requests.remove (i);
            break;
        }
    }
    startRequest();
}

int OpenStreetMaps::getMapWidthPixels (int zoom)
{
    double numberOfTiles = std::pow (2.0, zoom);
    return roundToInt (numberOfTiles * 256);
}

int OpenStreetMaps::getMapWidthTiles (int zoom)
{
    return roundToInt (std::pow (2.0, zoom));
}

Point<double> OpenStreetMaps::coordinateToDisplay (Point<double> coordinate, int zoom)
{
    double numberOfTiles = std::pow (2.0, zoom);

    // LonToX
    double x = (coordinate.getX() + 180) * (numberOfTiles * tilesize) / 360.0;
    // LatToY
    double projection = std::log (std::tan (double_Pi / 4 + degreesToRadians (coordinate.getY()) / 2));

    double y = (projection / double_Pi);
    y = 1 - y;
    y = y /2  * (numberOfTiles * tilesize);

    return Point<double> (x, y);
}

Point<double> OpenStreetMaps::displayToCoordinate (Point<double> point, int zoom)
{
    // longitude
    double longitude = (point.getX() * (360 / (std::pow (2.0, zoom) * 256))) - 180;
    // latitude
    double latitude = point.getY() * (2 / (std::pow (2.0, zoom) * 256));

    latitude = 1 - latitude;
    latitude = latitude * double_Pi;
    latitude = radiansToDegrees (std::atan (std::sinh (latitude)));

    Point<double> coord = {longitude, latitude};
    return coord;
}

Point<double> OpenStreetMaps::tileForCoordinate (double lat, double lng, int zoom)
{
    double zn = static_cast<double>(1 << zoom);
    double tx = (lng + 180.0) / 360.0;
    double ty = (1.0 - std::log (std::tan (lat * double_Pi / 180.0) + 1.0 / std::cos (lat * double_Pi / 180.0)) / double_Pi) / 2.0;
    return {tx * zn, ty * zn};
}

void OpenStreetMaps::clearQueue()
{
    while (requests.size() > 0)
        cancelledRequests.add (requests.removeAndReturn (0));

    serversInUse.clear();
}

OpenStreetMaps::TileSource OpenStreetMaps::getTileSource()
{
    return tileSource;
}

void OpenStreetMaps::setTileSource(TileSource t)
{
    tileSource = t;
}
