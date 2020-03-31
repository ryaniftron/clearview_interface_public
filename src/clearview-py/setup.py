import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="clearview-ryaniftron", # Replace with your own username
    version="0.0.1",
    author="Ryan Friedman",
    author_email="ryaniftron@gmail.com",
    description="Package for controlling and simulating a ClearView video reciever",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/ryaniftron/clearview_interface_public,
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 2",
        "License :: OSI Approved :: GNU GPLv3",
        "Operating System :: OS Independent",
    ],
    python_requires='>=2.7.9',

)



