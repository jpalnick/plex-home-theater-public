//
//  PlexAutoUpdate.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexAutoUpdate.h"
#include <boost/foreach.hpp>

CPlexAutoUpdate::CPlexAutoUpdate(const std::string &updateUrl, int searchFrequency) :
  m_updateUrl(updateUrl), m_searchFrequency(searchFrequency), m_stop(false), m_currentVersion("9.9.9.9")
{
  m_functions = new CAutoUpdateFunctionsXBMC(this);

  boost::thread t(boost::bind(&CPlexAutoUpdate::run, this));
  t.detach();
}

void CPlexAutoUpdate::Stop()
{
  m_stop = true;
  m_waitSleepCond.notify_all();
}

void CPlexAutoUpdate::run()
{
  boost::mutex::scoped_lock lk(m_lock);
  m_functions->LogDebug("Thread is running...");

  m_waitSleepCond.timed_wait(lk, boost::posix_time::seconds(5));

  while (!m_stop)
  {
    if (CheckForNewVersion())
    {
      DownloadNewVersion();
    }

    boost::system_time const tmout = boost::get_system_time() + boost::posix_time::seconds(m_searchFrequency);
    m_waitSleepCond.timed_wait(lk, tmout);
  }

  m_functions->LogDebug("Thread is going away");
}

bool CPlexAutoUpdate::DownloadNewVersion()
{
  return false;
}

bool CPlexAutoUpdate::CheckForNewVersion()
{
  std::string data;
  m_functions->LogInfo("Checking for new version");
  if (m_functions->FetchUrlData(m_updateUrl, data))
  {
    CAutoUpdateInfoList list;
    if (m_functions->ParseXMLData(data, list))
    {
      BOOST_FOREACH(CAutoUpdateInfo info, list)
      {
        if(m_currentVersion < info.m_enclosureVersion)
        {
          m_functions->LogInfo("Found new version " + info.m_enclosureVersion.GetVersionString());
          m_newVersion = info;
          m_functions->NotifyNewVersion();

          return true;
        }
      }
    }
  }
  m_functions->LogInfo("No new version found!");
  return false;
}
