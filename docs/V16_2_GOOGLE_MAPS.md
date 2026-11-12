# GREEN ION V16.2 — embedded Google Maps recovery

Recovery always keeps the offline GPS plot. The embedded Google Map is an optional internet feature.

## What it displays

Only **confirmed received GPS fixes** are used for the rocket trail/position. A reported landing phase never creates a coordinate.

When available the map shows:

- satellite view by default;
- standard, terrain, satellite and hybrid map types;
- confirmed rocket trail;
- latest/last-known rocket fix;
- fix age and source (Wi-Fi or LoRa);
- satellites;
- HDOP when recorded (dilution, not a meter-accuracy claim);
- your current browser location after explicit permission;
- distance/bearing through the existing local recovery calculation;
- an external Google Maps directions shortcut.

## API setup

1. In Google Cloud, create/select a project.
2. Enable the **Maps JavaScript API**.
3. Configure billing if Google requires it for the account/project.
4. Create a browser API key.
5. Restrict it to the Maps JavaScript API and to the origins you actually use.
6. Do **not** commit the key to Git.
7. For Windows local use, set `GOOGLE_MAPS_BROWSER_KEY` before launching GREEN ION.
8. For a hosted build, provide `GOOGLE_MAPS_BROWSER_KEY` only in the deployment environment.

If the key is missing, invalid, blocked or the internet is offline, Recovery stays functional through the local plot, coordinates, distance/bearing and external map link.

Your current browser position is not stored in flight history. Google may process map/location requests inherently required to provide its map/navigation service.
