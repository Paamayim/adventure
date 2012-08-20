import glob
import os
import re
import subprocess
import sys
from xml.dom.minidom import parse, parseString

def getTime(str, fps):
    ticks = 0
    seconds = re.search("([0-9.]+)s", str)
    if seconds: ticks += float(seconds.group(1)) * fps
    frames = re.search("([0-9.]+)f", str)
    if frames: ticks += float(frames.group(1))
    return int(ticks)

def getFrames(file):
    p = subprocess.Popen(["gzip", "-dfc", file], stdout=subprocess.PIPE)
    lines = p.stdout.readlines()

    f = open("obj/test.tmp", "w")
    for line in lines: f.write(line.decode("utf-8"))
    f.close()

    dom = parse("obj/test.tmp")
    dom = dom.getElementsByTagName("canvas")[0]
    
    fps = float(dom.getAttribute("fps"))
    width = int(dom.getAttribute("width"))
    height = int(dom.getAttribute("height"))
    
    endTime = getTime(dom.getAttribute("end-time"), fps)
    
    events = { }
    for keyframe in dom.getElementsByTagName("keyframe"):
        if keyframe.firstChild:
            events[getTime(keyframe.getAttribute("time"), fps)] = keyframe.firstChild.data
    
    return { 
            'file': "game/costumes/%%s/%s.png" % os.path.splitext(os.path.basename(file))[0],
            'fps': fps,
            'width': width,
            'height': height,
            'frames' : endTime,
            'events' : events
    }



print("-- this file is procedurally generated by utils/build_costume.py")
print("-- so you probably shouldn't edit it by hand :)\n")
print("local cost = nil")
print("local anim = nil")
print("local deck = nil")
print("local curve = nil")

for path in glob.iglob(sys.argv[1] + "/*"):
    if os.path.isdir(path):
        poses = { }
        anim = os.path.basename(path)
        
        for file in glob.iglob(path + "/*.sifz"):
            base = os.path.basename(file)
            base = os.path.splitext(base)[0]
            
            result = re.search("([a-z]+)([0-9]*)", base)
            pose = result.group(1)
            dir = result.group(2)
            
            if dir == "":
                dir = "5"
            
            if not pose in poses:
                poses[pose] = {}
                
            poses[pose][dir] = getFrames(file)
            
        print("cost = Costume.new()")
        print("cost.poses = {")
        for name, pose in poses.items():
            print("\t%s = { }," % name)
        print("\tnil")
        print("}")

        for name, pose in poses.items():
            for dir, data in pose.items():
                print("curve = MOAIAnimCurve.new()")
                print("curve:reserveKeys(%d)" % data["frames"])
                
                for i in range(1, data["frames"]):
                    print("curve:setKey(%d, %f, %d, MOAIEaseType.FLAT)" % (i, (i - 1) / data["fps"], i))
                print("curve:setKey(%d, 1, 1, MOAIEaseType.FLAT)" % data["frames"])

                print()
                firstpass = "anim = Animation.new(\"../%s\", %d, %d, %d, curve)" % (data["file"], data["frames"], data["width"] / 4, data["height"] / 4)
                print(firstpass % anim)
                
                #for time, event in data["events"].items():
                    #print("anim.events[%s] = \"%s\"" % (time, event))
                
                if name == "walk" or name == "idle":
                    print("anim.loops = true")
                    
                print("cost.poses.%s[%s] = anim" % (name, dir))
        
        print("costumes.%s = cost\n" % anim)
