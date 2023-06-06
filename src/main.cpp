#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <pcap-stdinc.h>
#endif

#include <pcap.h>
#include <vector>
#include <set>
#include <map>
#include <string.h>

#include <util.h>

#define DLT_NFLOG 239
#define DLT_ETH 1
#define _MOD (1000003)
#define _E (1993)


using namespace std;

typedef struct __flow_tuple{
	unsigned int src_ip = 0;
	unsigned int dst_ip = 0;
	unsigned short src_port = 0;
	unsigned short dst_port = 0;
	unsigned char protocol=0;
	int _hash = 1;
	__flow_tuple(unsigned int& _src_ip, unsigned int& _dst_ip, unsigned short& _src_port, unsigned short& _dst_port, unsigned char & _protocol)
	{
		char tuples[64] = { 0 };
		if (_src_ip > _dst_ip)
		//ȷ��C->S��S->C��Ԫ����Ϣ��һ�µ�
		{
			swap(_src_ip, _dst_ip);
			swap(_src_port, _dst_port);
		}
		src_ip = _src_ip;
		dst_ip = _dst_ip;
		src_port = _src_port;
		dst_port = _dst_port;
		protocol = _protocol;
		sprintf(tuples, "%u%u%u%u%c", _src_ip, _dst_ip, _src_port, _dst_port, protocol);
		for (int i = 0; i < 64; i++){
			_hash = (_hash * _E + tuples[i]) % _MOD;
		}
	}
	__flow_tuple();
} flow_tuple;
flow_tuple gather_flow_tuple( const unsigned char * data){
	
	ethII_header eth = eth_parser(data);

	unsigned int src_ip = 0;
	unsigned int dst_ip = 0;
	unsigned short src_port = 0;
	unsigned short dst_port = 0;
	unsigned char protocol = 0;

	if (eth.type == 0x0800)
		//ip Э��
	{
		ip_header ip = ip_parser(data + sizeof(ethII_header));
		src_ip = ip.saddr;
		dst_ip = ip.daddr;
		protocol = ip.proto;

		if (ip.proto == 0x11)
			//udp
		{
			udp_header udp = udp_parser(data + sizeof(ethII_header)+4 * (ip.ver_ihl & 0xF));
			src_port = udp.sport;
			dst_port = udp.dport;
		}
		else if (ip.proto == 0x06)
			//tcp
		{
			tcp_header tcp = tcp_parser(data + sizeof(ethII_header)+4 * (ip.ver_ihl & 0xF));
			src_port = tcp.sport;
			dst_port = tcp.dport;
		}
	}
	else{
		//��IPЭ��; ��MAC��ַ��4λ��ΪIP
		src_ip = * ((unsigned int *) eth.source);
		dst_ip = *((unsigned int *) eth.destination);
	}
	flow_tuple rst(src_ip, dst_ip, src_port, dst_port, protocol);
	return rst;
}
int splitpcaps(char *pcapname, char * dst_dir, int piece_num=10)
{

	pcap_t * rdpcap;//��pcap��ָ��
	
	char errBUF[4096] = { 0 };
	rdpcap = pcap_open_offline(pcapname,errBUF);
	if (rdpcap == NULL)
	{
		printf("Error when open pcap file. error is :%s\n", errBUF);
		return -1;
	}

	//����Ŀ¼
	char mkdir_cmd[256] = { 0 };
	sprintf(mkdir_cmd, "mkdir %s", dst_dir);
	system(mkdir_cmd);

	//���δ��� //дpcap��ָ��
	vector<pcap_dumper_t *> wtpcap_dumps;	
	vector<pcap_t *> _wtpcaps;

	for (int i = 0; i < piece_num; i++)
	{
		char dstfile[256] = { 0 };
		sprintf(dstfile, "%s/%d.pcap", dst_dir, i);
		pcap_t * wtpcap = pcap_open_dead(DLT_ETH, 65535);//��һ�������� linktype,��̫����linktype��1;
		pcap_dumper_t * wtpcap_dump = pcap_dump_open(wtpcap, dstfile);
		_wtpcaps.push_back(wtpcap);
		wtpcap_dumps.push_back(wtpcap_dump);
		
		//����Ƿ�򿪳ɹ�
		if (wtpcap == NULL || wtpcap_dump == NULL)
		{
			printf("Error when create dst file:%s\n", dstfile);
			return -2;
		}

	}

	int nb_transfer = 0;
	while (1)
	{
		pcap_pkthdr pktheader;
		const u_char *pktdata = pcap_next(rdpcap, &pktheader);
		if (pktdata != NULL)
			//pcap����pcapketδ����
		{
			//printf("Packet :%d\n", nb_transfer);

			//����packet��������
			display((unsigned char *)pktdata, pktheader.len);
			pcap_pkthdr new_pkthdr = pktheader;
			u_char new_data[2000] = { 0};
			memcpy(new_data, pktdata, new_pkthdr.len);
			//��ȡ��Ԫ����Ϣ,������Ԫ����й�ϣ
			flow_tuple tuple = gather_flow_tuple(pktdata);

			//д���Ӧ��Сpcap��
			pcap_dumper_t * wtpcap_dump = wtpcap_dumps[tuple._hash % piece_num];
			pcap_dump((u_char*) wtpcap_dump, &new_pkthdr, new_data);
			nb_transfer++;
			if (nb_transfer % 100 == 0)
			{
				for (int i = 0; i < wtpcap_dumps.size(); i++){
					pcap_dump_flush(wtpcap_dumps[i]);
				}
			}
		}
		else
		{
			break;
		}
	}
	//�ر�pcap���
	for (int i = 0; i < wtpcap_dumps.size(); i++){
		pcap_dump_flush(wtpcap_dumps[i]);
		if (wtpcap_dumps[i])
		{
			pcap_dump_close(wtpcap_dumps[i]);
		}
		if (_wtpcaps[i]){
			pcap_close(_wtpcaps[i]);
		}
	}
	if (rdpcap)
	{
		pcap_close(rdpcap);
	}
	return nb_transfer;
}
int main(int argc,char *argv[])
{
	//splitpcaps("C:\\Users\\dk\\splitpcap\\vsrc\\Debug\\nat.pcap","nat",10);
	//system("pause");
	//return 0;

	if (argc != 4)
	{
		printf("Split large PCAP file into multi smaller PCAP pieces.\n");
		printf("Usage:\n\t splitpcap src_pcapname dst_dir piece_num\n"\
			"\t\t src_pcapname: The src pcap to be splitted.\n"\
			"\t\t dst_dir: The dst directory to save the PCAP pieces\n"\
			"\t\t piece_num: The number of pieces pcaps.\n\n");
		exit(-1);
	}
	
	char * pcapname = argv[1];
	char * dst_dir = argv[2];
	int piece_num = atoi(argv[3]);

	if (splitpcaps(pcapname, dst_dir,piece_num) <0 )
	{
			printf("Error!!!!%s\n", pcapname);
	}

	//system("pause");
	return 0;
}
