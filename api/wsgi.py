from .app import app

#Iniciamos gunicorn desde comando en dockerfile
if __name__ == '__main__':
    app.run(use_reloader = False, debug = False)