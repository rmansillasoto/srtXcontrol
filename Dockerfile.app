FROM python:3.9.1-slim-buster

RUN apt-get update && apt-get install -y default-libmysqlclient-dev build-essential

WORKDIR /api
COPY ./requirements.txt /api/requirements.txt
RUN /usr/local/bin/python3 -m pip install --upgrade pip
RUN pip3 install -r requirements.txt
COPY ./api /api
RUN rm /api/requirements.txt

WORKDIR /

# create folder
#RUN mkdir /root/.docker
# create config with empty object
#RUN echo "{}" >  /root/.docker/config.json

#run gunicorn
CMD ["gunicorn", "-w", "3", "-b", ":5000", "-t", "360", "--reload", "api.wsgi:app"]
