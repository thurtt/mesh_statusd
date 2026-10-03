import sys
import time
import meshtastic.tcp_interface
import argparse

MAX_AGE = 2 * 3600  # only count nodes heard within the last 2 hours

if __name__ == "__main__":
    argparser = argparse.ArgumentParser()
    argparser.add_argument("--nodes", action='store_true' )
    argparser.add_argument("--gps", action='store_true' )
    argparser.add_argument("--host", type=str, default="localhost" )
    args = argparser.parse_args()

    iface = meshtastic.tcp_interface.TCPInterface(hostname=args.host)
    try:
        if args.nodes:
            my_num = iface.myInfo.my_node_num
            now = time.time()
            count = 0
            for node in iface.nodes.values():
                if node.get("num") == my_num:
                    continue  # skip own node
                heard = node.get("lastHeard", 0)
                if MAX_AGE and (now - heard) > MAX_AGE:
                    continue
                count += 1
            print(count)

        if args.gps:
            # DISABLED / ENABLED / NOT_PRESENT
            pos = iface.localNode.localConfig.position
            print(pos.GpsMode.Name(pos.gps_mode))
    finally:
        iface.close()
