#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include <set>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define INF 99999

using namespace std;

int socketfd;
int received_bytes;
int clockCycle = 0;

string IpAddress;

bool entry = false;

vector<string> neighbors;
set<string> routers;

struct RoutingTable
{
    string destination;
    string nextHop;
    int cost;
};

struct EdgeLink
{
    string neighborRout;
    int cost;
    int status;
    int receiveClock;
};

vector<RoutingTable> routingTable;
vector<EdgeLink> links;
vector<EdgeLink> uplinks;

void printTable()
{
    cout<<"\t------\t"<<IpAddress<<"\t------\t"<<endl;
    cout<<"Destination  \tNext Hop \tCost"<<endl;
    cout<<"-------------\t-------------\t-----"<<endl;
    for(int i = 0; i<routingTable.size(); i++)
    {
        if(!routingTable[i].destination.compare(IpAddress)) continue;
        cout<<routingTable[i].destination<<"\t"<<routingTable[i].nextHop<<"\t"<<routingTable[i].cost<<endl;
    }
    cout<<"--------------------------------------"<<endl;
}

bool isNeighbor(string neighborIp)
{
    for(int i = 0; i<links.size(); i++)
    {
        if(!neighborIp.compare(links[i].neighborRout))
            return true;
    }
    return false;
}

void initialRouter(string initrouterIp, string topology){
	//cout<<"hi"<<endl;
	ifstream topo(topology.c_str());
    	string r1, r2;
    	int cost;

    	while(!topo.eof()){

	        topo>>r1>>r2>>cost;

	        routers.insert(r1);
	        routers.insert(r2);
	
	        struct EdgeLink e;

	        if(!r1.compare(initrouterIp)){

			if(!isNeighbor(r2)){
			
            			neighbors.push_back(r2);
            			e.neighborRout = r2;
            			e.cost = cost;
				        e.status = 1;
				        e.receiveClock = 0;
            			links.push_back(e);
                        uplinks.push_back(e);
			         }
        	   }
        	else if(!r2.compare(initrouterIp))
        	{
			    if(!isNeighbor(r1))
			    {
		        	neighbors.push_back(r1);
		        	e.neighborRout = r1;
		        	e.cost = cost;
				    e.status = 1;
				    e.receiveClock = 0;
		        	links.push_back(e);
                    uplinks.push_back(e);
			    }
        	}
    	}

    	topo.close();

    	struct RoutingTable route;
    	string dest = "192.168.10.";
    	for(int i = 0; i<routers.size(); i++)
    	{
		    char x = i+1+'0';
        	string temp = dest + x;
        	if(find(neighbors.begin(),neighbors.end(),temp)!=neighbors.end())
        	{
        	    for(int j = 0; j<links.size(); j++)
        	    {
        	        if(!links[j].neighborRout.compare(temp))
        	        {
        	            route.destination = temp;
        	            route.nextHop = temp;
        	            route.cost = links[j].cost;
        	        }
        	    }
        	}
        	else if(!initrouterIp.compare(temp))
        	{
        	    route.destination = temp;
        	    route.nextHop = temp;
        	    route.cost = 0;
        	}
        	else
        	{
        	    route.destination = temp;
        	    route.nextHop = "\t-";
        	    route.cost = INF;
        	}
        	routingTable.push_back(route);
    	}	
    printTable();
}

struct RoutingTable getRoute(string row, string delim)
{
	char *str = new char[row.length()+1];
	strcpy(str,row.c_str());
	struct RoutingTable rt;
	vector<string> entries;
	char *stok = strtok(str,delim.c_str());

	while(stok!=NULL)
	{
		entries.push_back(stok);
		stok = strtok(NULL,delim.c_str());
	}

	rt.destination = entries[0];
	rt.nextHop = entries[1];
	rt.cost = atoi(entries[2].c_str());
	entries.clear();
	return rt;
}

string makePacketofTable(vector<RoutingTable> rt)
{
    string packet = "ntbl"+IpAddress;
    for(int i = 0; i<rt.size(); i++)
    {
        packet = packet + ":" + rt[i].destination + "#" + rt[i].nextHop + "#" + to_string(rt[i].cost);
    }
    return packet;
}

vector<RoutingTable> makeTableFromPacket(string packet)
{
    vector<RoutingTable> routingtab;
    vector<string> routentries;
    char *str = new char[packet.length()+1];
    strcpy(str, packet.c_str());
    char *token = strtok(str,":");
    while(token != NULL)
    {
        routentries.push_back(token);
        token = strtok(NULL,":");
    }
    struct RoutingTable rt;
    for(int i = 0; i<routentries.size(); i++)
    {
        rt = getRoute(routentries[i],"#");
        routingtab.push_back(rt);
        cout<<"dest : "<<rt.destination<<"  next : "<<rt.nextHop<<"  cost : "<<rt.cost<<endl;
    }
    return routingtab;
}

void sendRouterTable()
{
    string tablePacket = makePacketofTable(routingTable);
    for(int i = 0; i<neighbors.size(); i++)
    {
        struct sockaddr_in router_address;

        router_address.sin_family = AF_INET;
        router_address.sin_port = htons(4747);
        inet_pton(AF_INET,neighbors[i].c_str(),&router_address.sin_addr);

        int sent_bytes = sendto(socketfd, tablePacket.c_str(), 1024, 0, (struct sockaddr*) &router_address, sizeof(sockaddr_in));
        if(sent_bytes!=-1)
        {
            cout<<"routing table : "<<IpAddress<<" sent to : "<<neighbors[i]<<endl;
        }
    }
}

string convertToIP(unsigned char * rawip)
{
    int ipSeg[4];
    for(int i = 0; i<4; i++)
        ipSeg[i] = rawip[i];
    string ip = to_string(ipSeg[0])+"."+to_string(ipSeg[1])+"."+to_string(ipSeg[2])+"."+to_string(ipSeg[3]);
    return ip;
}

int getNeighbor(string neighborip)
{
    for(int i = 0;  i<links.size(); i++)
    {
        if(!neighborip.compare(links[i].neighborRout))
        {
            return i;
        }
    }
}

void updateForLinkFailure(string neighbor)
{
    int j = 0;
    for(int i = 0; i<routingTable.size(); i++)
    {
        if(!neighbor.compare(routingTable[i].nextHop))
        {
            if(!neighbor.compare(routingTable[i].destination) || !isNeighbor(routingTable[i].destination))
            {
                routingTable[i].nextHop = "\t-";
                routingTable[i].cost = INF;
                entry = true;
            }
            else if(isNeighbor(routingTable[i].destination))
            {
                routingTable[i].nextHop = routingTable[i].destination;
                routingTable[i].cost = links[getNeighbor(routingTable[i].destination)].cost;
                entry = true;
            }
        }
    }
    if(entry == true)
    {
        printTable();
    }
    entry = false;
}

void updateForNeighbor(string neighborip, vector<RoutingTable> neighborroutingtab)
{
    int tempCost;
    for(int i = 0; i<routers.size(); i++)
    {
        for(int j = 0; j<links.size(); j++)
        {
            if(!neighborip.compare(links[j].neighborRout))
            {
                tempCost = links[j].cost + neighborroutingtab[i].cost;
                if(!neighborip.compare(routingTable[i].nextHop)||((tempCost<routingTable[i].cost) && IpAddress.compare(neighborroutingtab[i].nextHop)!=0))
                {
                    if(routingTable[i].cost != tempCost)
                    {
                        routingTable[i].nextHop = neighborip;
                        routingTable[i].cost = tempCost;
                        entry = true;
                    }
                    break;
                }
            }
        }
    }
    if(entry==true){
        printTable();
    }
    entry = false;
}

void updateForCostChange(string neighbor, int newCost, int prevCost)
{
    for(int i = 0; i<routers.size(); i++)
    {
        if(!neighbor.compare(routingTable[i].nextHop))
        {
            if(!neighbor.compare(routingTable[i].destination))
            {
                routingTable[i].cost = newCost;
            }
            else
            {
                routingTable[i].cost = routingTable[i].cost - prevCost + newCost;
            }
            entry = true;
        }
        else if(!neighbor.compare(routingTable[i].destination) && routingTable[i].cost>newCost)
        {
            routingTable[i].cost = newCost;
            routingTable[i].nextHop = neighbor;
            entry = true;
        }
    }
    if(entry == true){
        printTable();
    }
    entry = false;
}

void forwardingMessage(string dest, string length, string msg)
{
    string forwardPacket = "frwd#"+dest+"#"+length+"#"+msg;
    string next_address;
    for(int i = 0; i<routers.size(); i++)
    {
        if(!dest.compare(routingTable[i].destination))
        {
            next_address = routingTable[i].nextHop;
            break;
        }
    }
    struct sockaddr_in router_address;

    router_address.sin_family = AF_INET;
    router_address.sin_port = htons(4747);
    inet_pton(AF_INET,next_address.c_str(),&router_address.sin_addr);

    int bytes_sent = sendto(socketfd, forwardPacket.c_str(), 1024, 0, (struct sockaddr*) &router_address, sizeof(sockaddr_in));
    cout<<msg.c_str()<<" packet forwarded to "<<next_address.c_str()<<" (printed by "<<IpAddress.c_str()<<")\n";
}

void receiveInput()
{
    struct sockaddr_in router_address;
    socklen_t addr_len;
    while(1){
        char buffer[1024];
        received_bytes = recvfrom(socketfd, buffer, 1024, 0, (struct sockaddr*) &router_address, &addr_len);
        if(received_bytes != -1)
        {
          //  cout<<"ok "<<endl;
            string recv(buffer);
            string head = recv.substr(0,4);
            if(!head.compare("show")){
                printTable();
            }
            else if(!head.compare("ntbl")){
                string neighborip = recv.substr(4,12);
                int index = getNeighbor(neighborip);
                links[index].status = 1;
                links[index].receiveClock = clockCycle;
                cout<<"receiver : "<<IpAddress<<" sender : "<<neighborip<<" receiving clock : "<<links[index].receiveClock<<endl;
                int length = recv.length()-15;
                char pack[length];
                for (int i=0; i<length; i++) {
                    pack[i] = buffer[16+i];
                }
                string packet(pack);
                vector<RoutingTable> ntbl = makeTableFromPacket(pack);
                updateForNeighbor(neighborip, ntbl); 
            }
            else if(!head.compare("send")){
                //forword message
                unsigned char *ipaddress1 = new unsigned char[5];
                unsigned char *ipaddress2 = new unsigned char[5];

                string temp1 = recv.substr (4, 4);
                string temp2 = recv.substr (8, 4);
                for (int i = 0; i < 4; i++)
                {
                    ipaddress1[i] = temp1[i];
                    ipaddress2[i] = temp2[i];
                }
                string socketip1 = convertToIP (ipaddress1);
                string socketip2 = convertToIP (ipaddress2);

                string msg_length = recv.substr(12,2);
                int length = 0;
                unsigned char *char1 = new unsigned char[3];
                char1[0] = msg_length[0];
                char1[1] = msg_length[1];

                int var1,var2;
                var1 = char1[0];
                var2 = char1[1]*256;
                length = var1 + var2;

                char msg[length+1];
                for (int i = 0; i < length; i++)
                {
                    msg[i] = buffer[14+i];
                }
                msg[length] = '\0';
                string message(msg);
                if(!socketip2.compare(IpAddress))
                {
                    cout<<message<<"packet reached destination. Printed by "<<socketip2<<endl;
                }
                else{
                    forwardingMessage(socketip2,msg_length,message);
                }
            }
            else if(!head.compare("frwd")){
                //forward to dest
                vector<string> fmsg;
                char *msg = new char[recv.length()+1];
                strcpy(msg, recv.c_str());
                char *token = strtok(msg, "#");
                while(token != NULL){
                    fmsg.push_back(token);
                    token = strtok(NULL, "#");
                }
                if(!fmsg[1].compare(IpAddress))
                {
                    cout<<fmsg[3]<<" packet reached destination. Printed by "<<fmsg[1]<<endl;
                }
                else{
                    forwardingMessage(fmsg[1],fmsg[2],fmsg[3]);
                }
                fmsg.clear();
            }
            else if(!head.compare("up")){
                unsigned char *ipaddress1 = new unsigned char[5];
                unsigned char *ipaddress2 = new unsigned char[5];

                string temp1 = recv.substr (4, 4);
                string temp2 = recv.substr (8, 4);
                for (int i = 0; i < 4; i++)
                {
                    ipaddress1[i] = temp1[i];
                    ipaddress2[i] = temp2[i];
                }
                string socketip1 = convertToIP (ipaddress1);
                string socketip2 = convertToIP (ipaddress2);

                int newCost;
                //cout<<newCost<<endl;
                string neighbor;
                for(int i=0; i<links.size(); i++)
                {
                    if(!socketip1.compare(links[i].neighborRout))
                    {
                        newCost = uplinks[i].cost;
                        links[i].cost = newCost;
                        neighbor = socketip1;
                    }
                    else if(!socketip2.compare(links[i].neighborRout))
                    {
                        newCost = uplinks[i].cost;
                        links[i].cost = newCost;
                        neighbor = socketip2;
                    }
                }
                updateForCostChange(neighbor, newCost, INF);
            }
            else if(!head.compare("down")){
                unsigned char *ipaddress1 = new unsigned char[5];
                unsigned char *ipaddress2 = new unsigned char[5];

                string temp1 = recv.substr (4, 4);
                string temp2 = recv.substr (8, 4);
                for (int i = 0; i < 4; i++)
                {
                    ipaddress1[i] = temp1[i];
                    ipaddress2[i] = temp2[i];
                }
                string socketip1 = convertToIP (ipaddress1);
                string socketip2 = convertToIP (ipaddress2);

                int prevCost;
                string neighbor;
                for(int i=0; i<links.size(); i++)
                {
                    if(!socketip1.compare(links[i].neighborRout))
                    {
                        prevCost = links[i].cost;
                        links[i].cost = INF;
                        neighbor = socketip1;
                    }
                    else if(!socketip2.compare(links[i].neighborRout))
                    {
                        prevCost = links[i].cost;
                        links[i].cost = INF;
                        neighbor = socketip2;
                    }
                }
                updateForCostChange(neighbor, INF, prevCost);
            }
            else if(!head.compare("cost")){
                //updated cost 
                unsigned char *ipaddress1 = new unsigned char[5];
                unsigned char *ipaddress2 = new unsigned char[5];

                string temp1 = recv.substr (4, 4);
                string temp2 = recv.substr (8, 4);
                for (int i = 0; i < 4; i++)
                {
                    ipaddress1[i] = temp1[i];
                    ipaddress2[i] = temp2[i];
                }
                string socketip1 = convertToIP (ipaddress1);
                string socketip2 = convertToIP (ipaddress2);

                string tempCost = recv.substr(12,2);

                cout<<tempCost<<endl;
                int newCost = 0;
                int prevCost;
                unsigned char *char1 = new unsigned char[3];
                char1[0] = tempCost[0];
                char1[1] = tempCost[1];

                int var1,var2;
                var1 = char1[0];
                var2 = char1[1]*256;
                newCost = var1 + var2;
                cout<<newCost<<endl;
                string neighbor;
                for(int i=0; i<links.size(); i++)
                {
                    if(!socketip1.compare(links[i].neighborRout))
                    {
                        prevCost = links[i].cost;
                        links[i].cost = newCost;
                        neighbor = socketip1;
                    }
                    else if(!socketip2.compare(links[i].neighborRout))
                    {
                        prevCost = links[i].cost;
                        links[i].cost = newCost;
                        neighbor = socketip2;
                    }
                }
                updateForCostChange(neighbor, newCost, prevCost);
            }
            else if(!head.compare("clk ")){
                clockCycle++;
                sendRouterTable();
                for(int i=0; i<links.size(); i++){
                    cout<<"clock cycle : "<<clockCycle<<endl;
                    if(clockCycle-links[i].receiveClock > 3 && links[i].status == 1)
                    {
                        cout<<"---------link down"<<links[i].neighborRout<<"---------"<<endl;
                        links[i].status = -1;
                        updateForLinkFailure(links[i].neighborRout);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]){
	
	if (argc != 3) {
        	cout<<"router : "<<argv[1]<<"<ip address>\n";
        	exit(1);
    	}
	IpAddress = argv[1];
	initialRouter(argv[1], argv[2]);

    struct sockaddr_in client_address;

    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(4747);
    inet_pton(AF_INET, argv[1], &client_address.sin_addr);

    socketfd = socket(AF_INET, SOCK_DGRAM, 0);

    int bind_flag;
    bind_flag = bind(socketfd, (struct sockaddr*) &client_address, sizeof(sockaddr_in));

    if(!bind_flag){
        cout<<"-------Connection successful-------"<<endl;
    }
    else{
        cout<<"------!! Connection failed !!------"<<endl;
    }

    receiveInput();

	return 0;
}
