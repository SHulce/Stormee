Stormee
=======
An emergency notification viewer

Summary
-------
This application parses xml files that adhere to the Common Alerting
Protocol (CAP), an OASIS standard adopted by FEMA, State, and local
agencies for providing emergency notifications to the public. CAP
notifications include details such as a timeframe, certainty,
severity, textual descriptions and instructions as well as maps and
web links for more information.

FEMA is developing a platform called 'IPAWS-OPEN' that aggregates CAP
messages for the United States and territories. Due to the
architecture adopted by FEMA, the public can not access this aggregate
service without signing a memorandum regarding terms of use. Stormee
is part of a system of two components: 1 component accesses FEMA's
aggregator and publishes it to an XMPP Pub/Sub node. This component is
in the 'ipaws' directory; it is not usable without a X509 certificate
provided by FEMA. The second component, Stormee, accesses the XMPP
Pub/Sub node and accepts alerts based on the user's location and
filter preferences.

UPDATE: It turns out IPAWS has no mechanism to retrieve alerts from
every publishing agency. And to retrieve alerts from any one agency,
one has to, in effect, be a member of that agency. So IPAWS aggregates
alerts for distribution to the general public, but does not actually
have any means to distribute them to the general public. Which is
stupid. Supposedly there will one day in mid-2012 be an RSS feed
available to the public. Until then, I'll be developing more
sporadically using California's EDIS feed. But I'm pretty bummed out
at the moment.

Stormee is in pre-alpha development.

Dependancies
------------
- cmake
- libxml2
- gtkmm3
- boost (Maybe not?)
- gettext
- libstrophe (You'll have to pull from my github branch of it for now)
- libchamplain

Current features
----------------
o Read CAP files from local disk
o Read CAP files from the XMPP Pub/Sub node
o stormee-bridge is connected to the IPAWS development server.
  - Alerts are really getting pushed to Stormee as they show up on
    IPAWS. Yay!
  - But all these alerts are just test messages.
  - I am now jumping through FEMA's hoops to get access to the
    production server.
o Provides basic desktop notification

Planned features
----------------
o Provide preferences to selecting location, filtering, etc
o Polish
  - Make the combobar colorified and pretty
  - Jazz up notifications
  - Display circles in map.
  - Display deref'd resources
  - Display image and other applicable resources inline
    - Need example CAPs to test before I'll bother with this
o Multiplatform

Technical Jargon
----------------
Stormee is a GTK+3 C++ application that interfaces with a XEP-0127
Pub/Sub server. It's purpose is to display Common Alerting Protocol
(CAP) messages in a human readable format; this means displaying
geographical information on maps and parsing codes back into English
(i.e. VTEC strings), and displaying any embedded resources (images,
links to videos, etc). In short, give the user as much information as
is available. XMPP communication is provided by libstrophe. This
component is licensed under the GPLv3+ license.

Stormee-bridge is a C application that uses the gSOAP library to
interface with IPAWS-OPEN 3. XMPP communication is provided by
libstrophe. This component is licensed under the GPLv2+ license (since
that's what gSOAP's machine generated code is under).

The XMPP server is ejabberd running on an Amazon EC2 node. (This
server is not available for public use)

Prototype
---------
The python directory has a python prototype for Stormee. It doesn't
interface with IPAWS but does have an RSS reader which reads from
California's EDIS feed (basically all the alerts in California). It
starts up as an applet, so just right click on it and click 'Execute'
to see it. An example is below:

### Textual Info
![Textual Info](https://raw.github.com/talisein/Stormee/master/imgs/info.jpg)


### Map display
![Map display](https://raw.github.com/talisein/Stormee/master/imgs/map.jpg)
