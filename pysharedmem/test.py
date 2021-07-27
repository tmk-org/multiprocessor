import dclprocessor

print(dclprocessor.test(3))

#def WorkWithObject(obj, boostpath, objname):
    #developer corner with object in shared memory
#    print("Hello, I'm in callback")
#    print("Sum of " + str(obj) + " elements is " +  objname)

#def pipeline_middle_process(_callback = None):
#    if _callback:
#        l = [1,2,3,4,5]
#        _callback(l, 0, str(dclprocessor.sum(l)))

#pipeline_middle_process(WorkWithObject)


def WorkWithObject(obj, boost_path, obj_name):
    #developer corner with object in shared memory
    print("python code: WorkWithObject")
    if dclprocessor.framesetsCount(obj) > 0:
        framesets = dclprocessor.framesetsIDs(obj)
        frameset = dclprocessor.frameset(obj, framesets[0])
        print("Frames number in the 1st framest of object: " + str(dclprocessor.framesCount(frameset)))
        frames = dclprocessor.framesIDs(frameset)
        frame = dclprocessor.frame(frameset, frames[0])
        image = dclprocessor.image(frame)
        print(image)
    else:
        print("No framesets in the " + obj_name)
    # developer corner with object in shared memory
    return 0

dclprocessor.pipeline_middle_process(WorkWithObject)
