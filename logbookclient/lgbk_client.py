#!/usr/bin/env python

import os
import sys
import logging
import requests
import json

logger = logging.getLogger(__name__)

def __parse_resp_json__(resp):
    """
    Parse the JSON from the response and return it. Log if there is any error...
    """
    try:
        return resp.json()
    except Exception as e:
        logger.error("Exception from the server; received %s", resp.text)
        logger.exception(e)
        raise e

class LogbookClient:
    def __init__(self, serverUrl, uid, passwd, useKerberos, verbose):
        self.serverUrl = serverUrl + "/" if not serverUrl.endswith("/") else serverUrl
        self.uid = uid
        self.passwd = passwd
        self.useKerberos = useKerberos
        self.authHeaders = { "auth": requests.auth.HTTPBasicAuth(self.uid, self.passwd) }

        root = logging.getLogger()
        ch = logging.StreamHandler(sys.stdout)
        if verbose:
            root.setLevel(logging.DEBUG)
            ch.setLevel(logging.DEBUG)
        else:
            root.setLevel(logging.INFO)
            ch.setLevel(logging.INFO)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        ch.setFormatter(formatter)
        root.addHandler(ch)

    def getActiveExperimentForInstrumentStation(self, instrument_name, station_num):
        logger.debug("Getting current active experiment for %s %s", instrument_name, station_num)
        resp = requests.get(self.serverUrl + "lgbk/ws/activeexperiment_for_instrument_station", params={"instrument_name" : instrument_name, "station": station_num}, **self.authHeaders)
        return __parse_resp_json__(resp)["value"]["name"]

    def getExperimentDetails(self, experiment_name):
        logger.debug("Getting experiment details for experiment %s ", experiment_name)
        resp = requests.get(self.serverUrl + "lgbk/{0}/ws/info".format(experiment_name), **self.authHeaders)
        info = __parse_resp_json__(resp)["value"]
        ret = {}
        ret["name"] = experiment_name
        ret["description"] = info.get("description", "")
        ret["proposal_id"] = info.get("params", {}).get("PNR", "")
        ret["leader_account"] = info.get("leader_account", "")
        ret["contact_info"] = info.get("contact_info", "")
        ret["posix_gid"] = info.get("posix_group", "")
        ret["instrument_name"] = info.get("instrument", "")
        return ret

    def getCurrentRun(self, experiment_name):
        logger.debug("Getting the current run for experiment %s ", experiment_name)
        resp = requests.get(self.serverUrl + "run_control/{0}/ws/current_run".format(experiment_name), **self.authHeaders)
        info = __parse_resp_json__(resp)
        if "value" in info and info["value"]:
            return int(info["value"]["num"])
        else:
            return None

    def startRun(self, experiment_name, run_type=None):
        logger.debug("Starting a new run for experiment %s ", experiment_name)
        resp = requests.get(self.serverUrl + "run_control/{0}/ws/start_run".format(experiment_name), params={"run_type": run_type} if run_type else None, **self.authHeaders)
        info = __parse_resp_json__(resp)
        return int(info["value"]["num"])

    def endRun(self, experiment_name):
        logger.debug("Ending the current run for experiment %s ", experiment_name)
        resp = requests.get(self.serverUrl + "run_control/{0}/ws/end_run".format(experiment_name), **self.authHeaders)
        info = __parse_resp_json__(resp)
        return int(info["value"]["num"])

    def addRunParams(self, experiment_name, params):
        logger.info("Adding %s run params for experiment %s ", len(params), experiment_name)
        resp = requests.post(self.serverUrl + "run_control/{0}/ws/add_run_params".format(experiment_name), json=params, **self.authHeaders)
        return __parse_resp_json__(resp)

    def registerFile(self, experiment_name, fileInfo):
        logger.debug("Registering file %s for experiment %s ", fileInfo["path"], experiment_name)
        resp = requests.post(self.serverUrl + "/lgbk/{0}/ws/register_file".format(experiment_name), json=fileInfo, **self.authHeaders)
        return __parse_resp_json__(resp)
