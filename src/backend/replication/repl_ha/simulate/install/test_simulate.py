import sys
import random
import time
import subprocess

class ha_site:
    def __init__(self, eid, binary, data_dir, addr, group_list_str):
        self.eid = eid
        self.addr = addr
        self.debugfile_name = str(eid) + '.debug'
        self.logfile_name = str(eid) + '.log'
        self.start_cmd = binary
        self.data_dir = data_dir
        self.group_list_str = group_list_str
        self.start = False

    def print_info(self):
        print 'Site[%d]:'%(self.eid)
        print '    addr: %s, debugfile: %s, logfile: %s'\
              %(self.addr, self.debugfile_name, self.logfile_name)
        print '    start cmd: %s, data dir: %s'%(self.start_cmd, self.data_dir)
        print '    group list: %s'%(self.group_list_str)

    def is_start(self):
        return self.start

    def start_site(self):
        if(self.is_start() == True):
            print 'site %d is already started'%(self.eid)
            return False
        else:
            print 'start site %d...'%(self.eid)

            #self.process = subprocess.Popen(self.start_cmd, shell = False)

            logfile = open(str(self.logfile_name), 'w')
            debugfile = open(str(self.debugfile_name), 'w')
            self.process = subprocess.Popen([self.start_cmd, \
                                self.addr, str(self.group_list_str)], \
                                bufsize = 1, \
                                shell = False,\
                                stdout = logfile,\
                                stderr = debugfile)
            debugfile.close()
            logfile.close()

            self.start = True
            return True

    def stop_site(self):
        if(self.is_start() == True):
            print 'stop site %d...'%(self.eid)
            self.process.kill()
            self.start = False
            return True
        else:
            print 'site %d is already stopped'%(self.eid)
            return False

    def truncate_log(self):
        logfile = open(str(self.logfile_name), 'w')
        print 'truncate file %s...'%(self.logfile_name)
        logfile.truncate()
        time.sleep(10)
        logfile.close()

    def truncate_debug(self):
        debugfile = open(str(self.debugfile_name), 'w')
        print 'truncate file %s...'%(self.debugfile_name)
        debugfile.truncate()
        time.sleep(10)
        logfile.close()

    def read_log(self):
        start = 0
        successed = 0
        master = 0
        heartbeat = 0

        logfile = open(str(self.logfile_name), 'r')
        for line in logfile:
            line_split = "".join(line.split())
            if(line_split == '#####START'):
                start += 1
            elif(line_split == '#####SUCCESSED'):
                successed += 1
            elif(line_split == '#####MASTER'):
                master += 1
            elif(line_split == '#####HEARTBEAT'):
                heartbeat += 1
            else:
                print 'read: [%s]'%(line)
        logfile.close()
        return (start, successed, master, heartbeat)

    def site_exist(self):
        if(self.is_start() == True):
            if(self.process.poll() == None):
                return True
        return False

class ha_group:
    def __init__(self, binary, data_dir, nsites, ipaddr):
        self.nsites = nsites
        self.data_dir = data_dir
        self.binary = binary
        self.my_ip = ipaddr
        self.first_port = 20000
        self.list_str = ''
        self.sites = []

        #set group list string
        for eid in range(nsites - 1):
            self.list_str += self.my_ip + ':%d;'%(self.first_port + eid)
        self.list_str += self.my_ip + ':%d'%(self.first_port + nsites - 1)

        #set all sites
        for eid in range(nsites):
            addr = self.my_ip + ':%s'%(str(self.first_port + eid))
            site = ha_site(eid, binary, data_dir, addr, self.list_str)
            self.sites.append(site)

    def print_info(self):
        print "=================================================="
        print 'data_dir: %s'%(self.data_dir)
        print 'binary: %s'%(self.binary)
        print 'nsites: %d, my ip: %s, first port: %d'\
              %(self.nsites, self.my_ip, self.first_port)
        print 'group list: %s'%(self.list_str)
        for eid in range(self.nsites):
            print ''
            self.sites[eid].print_info()
        print "=================================================="

    def rand_get_a_site(self):
        eid = random.randint(0, self.nsites - 1)
        #print 'Random get a site %d'%(eid)
        return eid

    def start_sites(self, nsites, interval_time):
        print 'Start to start %d sites[nsites: %d]...'%(nsites, self.nsites)
        nstart = 0
        ntries = 0
        while(nstart < nsites and ntries <= 1000):
            ntries += 1
            eid = self.rand_get_a_site()
            if(self.sites[eid].is_start() == False):
                if(self.sites[eid].start_site() == True):
                    nstart += 1
                    if(interval_time != 0):
                        time.sleep(interval_time)
                else:
                    print 'start %d sites failed.'%(nsites)
                    return False
        print 'Start %d sites successed.'%(nsites)
        return True

    def stop_sites(self, nsites, interval_time):
        print 'Start to stop %d sites[nsites: %d]...'%(nsites, self.nsites)
        nstop = 0
        ntries = 0
        while(nstop < nsites and ntries <= 1000):
            ntries += 1
            eid = self.rand_get_a_site()
            if(self.sites[eid].is_start() == True):
                if(self.sites[eid].stop_site() == True):
                    nstop += 1
                    if(interval_time != 0):
                        time.sleep(interval_time)
                else:
                    print 'Stop %d sites failed.'%(nsites)
                    return False
        print 'Stop %d sites successed.'%(nsites)
        return True

    def stop_all_sites(self):
        print 'Start to stop all started sites...'
        for eid in range(self.nsites):
            if(self.sites[eid].is_start() == True):
                if(self.sites[eid].stop_site() == False):
                    print 'Stop site[%d] failed.'%(eid)
                    return False
        return True

    def check_elect_fail(self):
        print 'Start to check election result[expect: failed]...'
        for eid in range(self.nsites):
            if(self.sites[eid].is_start() == True):
                #check site exist or not
                if(self.sites[eid].site_exist() == False):
                    print 'site[%d] is down!!!!!!!!!!!!'%(eid)
                    return False

                #check site log
                (start, successed, master, heartbeat) = self.sites[eid].read_log()
                if(start == 1 and successed == 0 and master == 0 and heartbeat == 0):
                    print 'site[%d] log is right.'%(eid)
                else:
                    print 'site[%d] log is wrong.'%(eid)
                    print 'Result is not the same as expect[Election failed].'
                    return False
        print 'Result is the same as expect[Election failed].'
        return True

    def check_elect_successed(self):
        print 'Start to check election result[expect: successed]...'
        invalid_eid = 65535
        master_eid = invalid_eid
        for eid in range(self.nsites):
            if(self.sites[eid].is_start() == True):
                #check site exist or not
                if(self.sites[eid].site_exist() == False):
                    print 'site[%d] is down!!!!!!!!!!!!'%(eid)
                    return False

                #check site log
                (start, successed, master, heartbeat) = self.sites[eid].read_log()
                if(start == 1 and successed == 1 and heartbeat == 0):
                    if(master == 1):
                        if(master_eid == invalid_eid):
                            print 'site[%d] is master.'%(eid)
                            master_eid = eid
                        else:
                            print 'two master: %d and %d.'%(eid, master_eid)
                            print 'Result is not the same as expect[Election successed].'
                            return (False, invalid_eid)
                    elif(master == 0):
                        print 'site[%d] is slave.'%(eid)
                    else:
                        print 'site[%d] log is wrong.'%(eid)
                        print 'Result is not the same as expect[Election successed].'
                        return (False, invalid_eid)
                elif(heartbeat == 1 and start == 0 and master == 0 and successed == 0):
                    print 'site[%d] is slave.'%(eid)
                else:
                    print 'site[%d] log is wrong.'%(eid)
                    print 'Result is not the same as expect[Election successed].'
                    return (False, invalid_eid)

        #truncate log & debug, after election successed
        #for eid in range(self.nsites):
        #    if(self.sites[eid].is_start() == True):
        #        self.sites[eid].truncate_log()
        #        self.sites[eid].truncate_debug()
        print 'Result(Master: %d) is the same as expect'\
                  '[Election successed].'%(master_eid)
        return (True, master_eid)

def test_case_stop(group, first_nsites, stop_time_interval, check_time):
    half_nsites = group.nsites / 2
    if(first_nsites < half_nsites + 1):
        print 'Wrong first start sites %d[nsites: %d].'\
              %(first_nsites, group.nsites)

    #first start first_nsites and check log
    if(group.start_sites(first_nsites, stop_time_interval) == False):
        group.stop_all_sites()
        return False
    time.sleep(check_time)
    print 'After %ds, Check election......'%(check_time)
    (successed, master_eid) = group.check_elect_successed()
    if(successed == False):
        group.stop_all_sites()
        return False

    #stop 1 sites at a time and check log, until half nsites + 1
    total_start_sites = first_nsites
    while(total_start_sites > half_nsites + 1):
        print 'total : ' + str(total_start_sites)
        if(group.sites[master_eid].stop_site() == False):
            group.stop_all_sites()
            return False

        total_start_sites -= 1

        #check election result
        time.sleep(check_time)
        (successed, master_eid) = group.check_elect_successed()
        if(successed == False):
            group.stop_all_sites()
            return False

    group.stop_all_sites()
    return True

def test_case_start(group, first_nsites, start_time_interval, check_time):
    half_nsites = group.nsites / 2
    if(first_nsites > half_nsites):
        print 'Wrong first start sites %d[nsites: %d].'\
              %(first_nsites, group.nsites)

    #first start first_nsites and check log
    if(group.start_sites(first_nsites, start_time_interval) == False):
        group.stop_all_sites()
        return False
    time.sleep(check_time)
    print 'After %ds, Check election......'%(check_time)
    if(group.check_elect_fail() == False):
        group.stop_all_sites()
        return False

    #start 1 site at a time and check log, until half nsites
    total_start_sites = first_nsites
    while(total_start_sites < half_nsites):
        if(group.start_sites(1, start_time_interval) == False):
            group.stop_all_sites()
            return False

        time.sleep(check_time)
        print 'After %ds, Check election......'%(check_time)
        if(group.check_elect_fail() == False):
            group.stop_all_sites()
            return False
        total_start_sites += 1

    #start sites and check log, until nsites
    remain_nsites = group.nsites - half_nsites
    for nsites in range(1, remain_nsites + 1):
        if(group.start_sites(nsites, start_time_interval) == False):
            group.stop_all_sites()
            return False

        time.sleep(check_time)
        print 'After %ds, Check election......'%(check_time)
        (successed, master_eid) = group.check_elect_successed()
        if(successed == False):
            group.stop_all_sites()
            return False
        #stop master and nsites - 1
        if(group.sites[master_eid].stop_site() == False):
            group.stop_all_sites()
            return False
        if(group.stop_sites(nsites - 1, int(0)) == False):
            group.stop_all_sites()
            return False

    #start 2 sites, stop master. if group.nsites <= 4, doesn't need test
    if(group.nsites <= 4):
        group.stop_all_sites()
        return True
    for nsites in range(1, remain_nsites + 1):
        if(nsites == 1):
            if(group.start_sites(1, start_time_interval) == False):
                group.stop_all_sites()
                return False
        else:
            if(group.start_sites(2, start_time_interval) == False):
                group.stop_all_sites()
                return False

        time.sleep(check_time)
        print 'After %ds, Check election......'%(check_time)
        (successed, master_eid) = group.check_elect_successed()
        if(successed == False):
            group.stop_all_sites()
            return False
        if(group.sites[master_eid].stop_site() == False):
            group.stop_all_sites()
            return False

    #stop all sites, and end test
    group.stop_all_sites()
    return True

def test_case_start_once(group, start_nsites, start_time_interval, check_time):
    if(group.start_sites(start_nsites, start_time_interval) == False):
        group.stop_all_sites()
        return False

    time.sleep(check_time)
    print 'After %ds, Check election......'%(check_time)
    if(start_nsites > group.nsites / 2):
        (successed, master_eid) = group.check_elect_successed()
        if(successed == False):
            group.stop_all_sites()
            return False
    else:
        if(group.check_elect_fail() == False):
            group.stop_all_sites()
            return False

    group.stop_all_sites()
    return True

def main():
    print ''
    print ''
    print 'Start main......'
    binary = sys.argv[1]
    data_dir = sys.argv[2]
    nsites = int(sys.argv[3])
    election_time = int(sys.argv[4])
    ipaddr = sys.argv[5]
    group = ha_group(binary, data_dir, nsites, ipaddr)
    group.print_info()

    #start once test
    start_nsites = 1
    while(start_nsites <= group.nsites):
        print ''
        print ''
        print '-------case: start once %d, interval: 0s[nsites:%d]----------'\
              %(start_nsites, group.nsites)
        if(test_case_start_once(group, start_nsites, 0, election_time) == False):
            print '====test failed.'
            group.stop_all_sites()
            sys.exit(1) 
        else:
            print '====test successed.'
        print '------------------------------------------------------------'
        start_nsites += 1
    
    #start all sites, and stop 1 site at a time
    print ''
    print ''
    print '----------case: stop %d, interval: 0s[nsites:%d]------------'\
          %(group.nsites, group.nsites)
    if(test_case_stop(group, group.nsites, 0, election_time) == False):
        print 'test failed.'
        group.stop_all_sites()
        sys.exit(1) 
    else:
        print 'test successed.'
    print '------------------------------------------------------------'
    
    #test start_stop
    print ''
    print ''
    print '----------case: start %d, interval: 0s[nsites:%d]-----------'\
          %(group.nsites / 2, group.nsites)
    if(test_case_start(group, group.nsites / 2, 0, election_time) == False):
        print 'test failed.'
        group.stop_all_sites()
        sys.exit(1) 
    else:
        print 'test successed.'
    print '------------------------------------------------------------'

    #test start_stop
    print ''
    print ''
    print '----------case: start %d, interval: 10s[nsites:%d]----------'\
          %(group.nsites / 2, group.nsites)
    if(test_case_start(group, group.nsites / 2, 10, election_time) == False):
        print 'test failed.'
        group.stop_all_sites()
        sys.exit(1) 
    else:
        print 'test successed.'
    print '------------------------------------------------------------'
    
    sys.exit(0)
    print '===========All test successed=============='
    

def usage():
    print 'Usage:'
    print sys.argv[0] + ' [<binary> <data_dir> <nsites> <election_time> <my_ip>]'

def parse_args():
    if len(sys.argv) == 6:
        for argc in range(len(sys.argv)):
            print sys.argv[argc]

        #check binary exist

        #check data_dir exist

        #check nsites
    else:
        usage()
        sys.exit(1)

if __name__ == '__main__':
    parse_args()
    main()
